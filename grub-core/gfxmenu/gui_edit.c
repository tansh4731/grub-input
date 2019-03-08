/* gui_label.c - GUI component to display a line of text.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/gui.h>
#include <grub/font.h>
#include <grub/gui_string_util.h>
#include <grub/i18n.h>
#include <grub/color.h>
#include <grub/input.h>

#define GRUB_TERM_DEFAULT_NORMAL_COLOR 0x07
#define GRUB_TERM_DEFAULT_HIGHLIGHT_COLOR 0x70
#define GRUB_TERM_DEFAULT_STANDARD_COLOR 0x07

struct grub_gui_edit
{
    struct grub_gui_component comp;

    grub_gui_container_t parent;

    grub_video_rect_t bounds;
    char *id;
    int index;
    int visible;
    char password_char;

    int padding_h; //水平
    int padding_v; //垂直
    int cursor_size;
    char *cursor_derect;
    char *marked_words;

    int cursor_width;
    int cursor_height;

    grub_font_t font;
    grub_video_rgba_color_t color;
    grub_video_rgba_color_t marked_words_color;

    char *color_text;
    char *marked_words_color_text;

    grub_gui_component_t label_text;
#ifdef EDIT_IMAGE_BACKGROUD
    grub_gui_component_t image_background;
#endif
};

typedef struct grub_gui_edit *grub_gui_edit_t;

static void
edit_destroy(void *vself)
{
    grub_gui_edit_t self = vself;
    self->label_text->ops->destroy(self->label_text);
#ifdef EDIT_IMAGE_BACKGROUD
    self->image_background->ops->destroy(self->image_background);
#endif
}

static const char *
edit_get_id(void *vself)
{
    grub_gui_edit_t self = vself;
    return self->id;
}

static int
edit_is_instance(void *vself __attribute__((unused)), const char *type)
{
    return (grub_strcmp(type, "component") == 0 || grub_strcmp(type, "input") == 0);
}

static void
edit_paint_background(void *vself, const grub_video_rect_t *region __attribute__((unused)))
{
    grub_gui_edit_t self = vself;

#ifdef EDIT_IMAGE_BACKGROUD
    self->image_background->ops->paint(self->image_background, region);
#else
    grub_video_rgba_color_t color;
    grub_video_parse_color("#FFFFFF", &color);
    grub_video_rgba_color_t color_side;
    grub_video_parse_color("#CFCFCF", &color_side);
    grub_video_rgba_color_t color_side2;
    grub_video_parse_color("#8F8F8F", &color_side2);
    grub_video_fill_rect(grub_video_map_rgba_color(color),
                         self->bounds.x,
                         self->bounds.y,
                         self->bounds.width,
                         self->bounds.height);
    
    //内边框
    grub_video_fill_rect(grub_video_map_rgba_color(color_side2),
                         self->bounds.x,
                         self->bounds.y + 1,
                         self->bounds.width - 2,
                         1);
    grub_video_fill_rect(grub_video_map_rgba_color(color_side2),
                         self->bounds.x + 1,
                         self->bounds.y,
                         1,
                         self->bounds.height - 2);
    grub_video_fill_rect(grub_video_map_rgba_color(color_side2),
                         self->bounds.x + self->bounds.width - 2,
                         self->bounds.y,
                         1,
                         self->bounds.height - 2);
    grub_video_fill_rect(grub_video_map_rgba_color(color_side2),
                         self->bounds.x,
                         self->bounds.y + self->bounds.height - 2,
                         self->bounds.width - 2,
                         1);

    //外边框
    grub_video_fill_rect(grub_video_map_rgba_color(color_side),
                         self->bounds.x,
                         self->bounds.y,
                         self->bounds.width,
                         1);
    grub_video_fill_rect(grub_video_map_rgba_color(color_side),
                         self->bounds.x,
                         self->bounds.y,
                         1,
                         self->bounds.height);
    grub_video_fill_rect(grub_video_map_rgba_color(color_side),
                         self->bounds.x + self->bounds.width - 1,
                         self->bounds.y,
                         1,
                         self->bounds.height);
    grub_video_fill_rect(grub_video_map_rgba_color(color_side),
                         self->bounds.x,
                         self->bounds.y + self->bounds.height - 1,
                         self->bounds.width,
                         1);
#endif
}

static void
edit_paint(void *vself, const grub_video_rect_t *region)
{
    input_log("edit_paint\n");

    grub_gui_edit_t self = vself;
    char show_text[128] = {0};
    unsigned single_char_width = 0;
    int show_marked_words = 0;
    unsigned width = 0;
    unsigned height = 0;

#if 0
    //single_char_width = grub_font_get_string_width(self->font, "L");
#else
    self->label_text->ops->set_property(self->label_text,
                                        "text", "*");
    self->label_text->ops->get_minimal_size(self->label_text, &single_char_width, &height);
#endif
    input_log("edit_paint: width: %d\n", single_char_width);

    edit_paint_background(vself, region);

    unsigned max_num_char_show = 0;
    if (grub_strcmp(self->cursor_derect, "vertical") == 0)
    {
        max_num_char_show = ((self->bounds.width - self->padding_h * 2 - self->cursor_size) / single_char_width); //不包括光标
    }
    else
    {
        max_num_char_show = ((self->bounds.width - self->padding_h * 2) / single_char_width) - 1; //不包括光标
    }

    if (max_num_char_show >= grub_strlen(g_current_input_text[self->index]))
    {
        grub_memcpy(show_text, g_current_input_text[self->index],
                    grub_strlen(g_current_input_text[self->index]));
    }
    else
    {
        grub_memcpy(show_text, g_current_input_text[self->index] + (grub_strlen(g_current_input_text[self->index]) - max_num_char_show),
                    max_num_char_show);
    }

    if (grub_strlen(show_text) == 0 && grub_strlen(self->marked_words) > 0)
    {
        self->label_text->ops->set_property(self->label_text,
                                            "color", self->marked_words_color_text);
        grub_memcpy(show_text, self->marked_words, grub_strlen(self->marked_words));

        show_marked_words = 1;
    }
    else
    {
        self->label_text->ops->set_property(self->label_text,
                                            "color", self->color_text);
        if (self->password_char)
        {
            grub_memset(show_text, self->password_char, grub_strlen(show_text));
        }
    }

    self->label_text->ops->set_property(self->label_text,
                                        "text", show_text);
    self->label_text->ops->paint(self->label_text, region);

    if (g_input_show_cursor && (self->index == g_current_input_text_index))
    {
        width = 0;
        height = 0;
        self->label_text->ops->get_minimal_size(self->label_text, &width, &height);

        input_log("edit_paint: width2: %s\n", show_text);
        input_log("edit_paint: width3: %d, %d\n", self->bounds.width, width);

        int cursor_x = self->bounds.x + width + self->padding_h;
        if (show_marked_words)
        {
            cursor_x = self->bounds.x + self->padding_h;
        }

        if (grub_strcmp(self->cursor_derect, "vertical") == 0)
        {

            grub_video_fill_rect(grub_video_map_rgba_color(self->color),
                                 cursor_x,
                                 self->bounds.y + self->padding_v,
                                 self->cursor_size, height);
        }
        else
        {
            grub_video_fill_rect(grub_video_map_rgba_color(self->color),
                                 cursor_x,
                                 self->bounds.y + self->padding_v + height - self->cursor_size,
                                 single_char_width, self->cursor_size);
        }
    }
}

static void
edit_set_parent(void *vself, grub_gui_container_t parent)
{
    grub_gui_edit_t self = vself;
    self->parent = parent;
}

static grub_gui_container_t
edit_get_parent(void *vself)
{
    grub_gui_edit_t self = vself;

    return self->parent;
}

static void
edit_set_bounds(void *vself, const grub_video_rect_t *bounds)
{
    grub_gui_edit_t self = vself;
    unsigned int minimal_height = 0;
    unsigned int minimal_width = 0;

    struct grub_video_rect label_bounds;

    self->label_text->ops->get_minimal_size(self->label_text, &minimal_width,
                                            &minimal_height);

    label_bounds.x = bounds->x + self->padding_h;
    label_bounds.y = bounds->y + self->padding_v;
    label_bounds.width = bounds->width - (self->padding_h * 2);
    label_bounds.height = minimal_height + (self->padding_v * 2);

    self->label_text->ops->set_bounds(self->label_text, &label_bounds);

    self->bounds.x = bounds->x;
    self->bounds.y = bounds->y;
    self->bounds.width = bounds->width;
    self->bounds.height = label_bounds.height;

#ifdef EDIT_IMAGE_BACKGROUD
    self->image_background->ops->set_bounds(self->image_background, self->bounds);
#endif
}

static void
edit_get_bounds(void *vself, grub_video_rect_t *bounds)
{
    grub_gui_edit_t self = vself;
    *bounds = self->bounds;
}

static void
edit_get_minimal_size(void *vself, unsigned *width, unsigned *height)
{
    grub_gui_edit_t self = vself;
    *width = self->bounds.width;
    *height = self->bounds.height;
}

#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static grub_err_t
edit_set_property(void *vself, const char *name, const char *value)
{
    grub_gui_edit_t self = vself;

    if (grub_strcmp(name, "color") == 0)
    {
        grub_video_parse_color(value, &self->color);
        grub_free(self->color_text);
        self->color_text = grub_strdup(value);
    }
    else if (grub_strcmp(name, "font") == 0)
    {
        self->font = grub_font_get(value);
    }
    else if (grub_strcmp(name, "marked_words_color") == 0)
    {
        grub_video_parse_color(value, &self->marked_words_color);
        grub_free(self->marked_words_color_text);
        self->marked_words_color_text = grub_strdup(value);
    }
    else if (grub_strcmp(name, "index") == 0)
    {
        self->index = grub_strtol(value, 0, 10);
    }
    else if (grub_strcmp(name, "password_char") == 0)
    {
        self->password_char = value[0];
    }
    else if (grub_strcmp(name, "id") == 0)
    {
        grub_free(self->id);
        if (value)
            self->id = grub_strdup(value);
        else
            self->id = grub_strdup("edit");
    }
    else if (grub_strcmp(name, "padding_h") == 0)
    {
        self->padding_h = grub_strtol(value, 0, 10);
    }
    else if (grub_strcmp(name, "padding_v") == 0)
    {
        self->padding_v = grub_strtol(value, 0, 10);
    }
    else if (grub_strcmp(name, "cursor_size") == 0)
    {
        self->cursor_size = grub_strtol(value, 0, 10);
    }
    else if (grub_strcmp(name, "cursor_derect") == 0)
    {
        grub_free(self->cursor_derect);
        if (value)
            self->cursor_derect = grub_strdup(value);
        else
            self->cursor_derect = grub_strdup("horizontal");
    }
    else if (grub_strcmp(name, "marked_words") == 0)
    {
        grub_free(self->marked_words);
        if (value)
            self->marked_words = grub_strdup(value);
        else
            self->marked_words = grub_strdup("");
    }

    self->label_text->ops->set_property(self->label_text, name, value);
#ifdef EDIT_IMAGE_BACKGROUD
    self->image_background->ops->set_property(self->image_background, name, value);
#endif
    return GRUB_ERR_NONE;
}

#pragma GCC diagnostic error "-Wformat-nonliteral"

static struct grub_gui_component_ops edit_ops =
    {
        .destroy = edit_destroy,
        .get_id = edit_get_id,
        .is_instance = edit_is_instance,
        .paint = edit_paint,
        .set_parent = edit_set_parent,
        .get_parent = edit_get_parent,
        .set_bounds = edit_set_bounds,
        .get_bounds = edit_get_bounds,
        .get_minimal_size = edit_get_minimal_size,
        .set_property = edit_set_property};

grub_gui_component_t
grub_gui_edit_new(void)
{
    grub_gui_edit_t edit;
    edit = grub_zalloc(sizeof(*edit));
    if (!edit)
        return 0;

    edit->comp.ops = &edit_ops;
    edit->index = 0;
    edit->password_char = 0;
    edit->padding_h = 10;
    edit->padding_v = 5;
    edit->cursor_size = 1;
    edit->cursor_derect = grub_strdup("horizontal");
    edit->marked_words = grub_strdup("");

    edit->label_text = grub_gui_label_new();
#ifdef EDIT_IMAGE_BACKGROUD
    edit->image_background = grub_gui_image_new();
#endif

    return (grub_gui_component_t)edit;
}
