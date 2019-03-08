/* menu_viewer.h - Interface to menu viewer implementations. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#ifndef GRUB_MENU_VIEWER_HEADER
#define GRUB_MENU_VIEWER_HEADER 1

#include <grub/err.h>
#include <grub/symbol.h>
#include <grub/types.h>
#include <grub/input.h>
#include <grub/term.h>

struct grub_input_viewer
{
  struct grub_input_viewer *next;
  void *data;
  void (*redraw_input) (void *data);
  void (*fini) (void *fini);
};

void grub_input_register_viewer (struct grub_input_viewer *viewer);

extern grub_err_t (*grub_gfxinput_try_hook) (int nested);

#endif /* GRUB_MENU_VIEWER_HEADER */
