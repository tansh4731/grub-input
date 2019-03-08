/* gfxmenu.c - Graphical menu interface controller. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/video.h>
#include <grub/gfxterm.h>
#include <grub/bitmap.h>
#include <grub/bitmap_scale.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/normal.h>
#include <grub/gfxwidgets.h>
#include <grub/input.h>
#include <grub/input_viewer.h>
#include <grub/gfxmenu_view.h>
#include <grub/time.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE("GPLv3+");

static grub_gfxmenu_view_t cached_view;

/* FIXME: Previously 't' changed to text menu is it necessary?  */
static grub_err_t
grub_gfxinput_try(int nested)
{
  grub_gfxmenu_view_t view = NULL;
  const char *theme_path;
  char *full_theme_path = 0;
  struct grub_input_viewer *instance;
  grub_err_t err;
  struct grub_video_mode_info mode_info;

  theme_path = grub_env_get("theme_input");
  if (!theme_path)
    return grub_error(GRUB_ERR_FILE_NOT_FOUND, N_("variable `%s' isn't set"),
                      "theme_input");

  err = grub_video_get_info(&mode_info);
  if (err)
    return err;

  instance = grub_zalloc(sizeof(*instance));
  if (!instance)
    return grub_errno;

  if (theme_path[0] != '/' && theme_path[0] != '(')
  {
    const char *prefix;
    prefix = grub_env_get("prefix");
    full_theme_path = grub_xasprintf("%s/themes/%s",
                                     prefix,
                                     theme_path);
  }

  if (!cached_view || grub_strcmp(cached_view->theme_path, full_theme_path ?: theme_path) != 0 || cached_view->screen.width != mode_info.width || cached_view->screen.height != mode_info.height)
  {
    grub_gfxmenu_view_destroy(cached_view);
    /* Create the view.  */
    cached_view = grub_gfxmenu_view_new(full_theme_path ?: theme_path,
                                        mode_info.width,
                                        mode_info.height);
  }
  grub_free(full_theme_path);

  if (!cached_view)
  {
    grub_free(instance);
    return grub_errno;
  }

  view = cached_view;

  view->double_repaint = (mode_info.mode_type & GRUB_VIDEO_MODE_TYPE_DOUBLE_BUFFERED) && !(mode_info.mode_type & GRUB_VIDEO_MODE_TYPE_UPDATING_SWAP);
  view->selected = 0;
  view->menu = NULL;
  view->nested = nested;
  view->first_timeout = -1;

  grub_video_set_viewport(0, 0, mode_info.width, mode_info.height);
  if (view->double_repaint)
  {
    grub_video_swap_buffers();
    grub_video_set_viewport(0, 0, mode_info.width, mode_info.height);
  }

  grub_gfxmenu_view_draw(view);

  instance->data = view;
  instance->fini = NULL;
  instance->redraw_input = grub_gfxmenu_redraw_input;

  grub_input_register_viewer(instance);

  return GRUB_ERR_NONE;
}

void grub_gfxinput_init(void)
{
  struct grub_term_output *term;

  FOR_ACTIVE_TERM_OUTPUTS(term)
  if (grub_gfxinput_try_hook && term->fullscreen)
  {
    term->fullscreen();
    break;
  }

  grub_gfxinput_try_hook = grub_gfxinput_try;
}

void grub_gfxinput_deinit(void)
{
  grub_gfxmenu_view_destroy(cached_view);
  grub_gfxinput_try_hook = NULL;
}
