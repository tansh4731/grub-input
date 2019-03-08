/* menu.c - General supporting functionality for menus.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/misc.h>
#include <grub/loader.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/env.h>
#include <grub/input_viewer.h>
#include <grub/command.h>
#include <grub/parser.h>
#include <grub/auth.h>
#include <grub/i18n.h>
#include <grub/term.h>
#include <grub/script_sh.h>
#include <grub/gfxterm.h>
#include <grub/dl.h>
#include <grub/input.h>

grub_err_t (*grub_gfxinput_try_hook)(int nested) = NULL;

// 输入框输入的接收字符串
char g_current_input_text[MAX_INPUT_NUM][MAX_INPUT_TEXT_LEN + 1] = {0};
// 当前输入框的index
int g_current_input_text_index = 0;
// 当前info框
char g_current_input_label_info[MAX_TEXT_INFO_LEN] = {0};
// 当前error框
char g_current_input_label_error[MAX_TEXT_INFO_LEN] = {0};
// input的数量
long g_input_edit_num = 0;
// 是否显示光标
int g_input_show_cursor = 1;

char g_current_input_log[1024 * 1024] = {0};

static grub_uint64_t s_saved_time = 0;

int (*input_check_password)(const char *password, char *error, int len) = NULL;

void input_log(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    grub_vsnprintf(g_current_input_log + grub_strlen(g_current_input_log),
                   sizeof(g_current_input_log) - grub_strlen(g_current_input_log),
                   _(fmt), ap);
    va_end(ap);
}

static void
input_init(int nested)
{
    struct grub_term_output *term;

    FOR_ACTIVE_TERM_OUTPUTS(term)
    if (term->fullscreen)
    {
        if (grub_env_get("theme_input"))
        {
            if (!grub_gfxinput_try_hook)
            {
                grub_dl_load("gfxmenu");
                grub_dl_load("xdjakey");
                grub_print_error();
            }
            if (grub_gfxinput_try_hook)
            {
                grub_err_t err = GRUB_ERR_NONE;
                err = grub_gfxinput_try_hook(nested);
                if (!err)
                {
                    break;
                }
            }
            else
                grub_error(GRUB_ERR_BAD_MODULE,
                           N_("module `%s' isn't loaded"),
                           "gfxmenu");
            grub_print_error();
            grub_wait_after_message();
        }
        grub_errno = GRUB_ERR_NONE;
        term->fullscreen();
        break;
    }
}

/* Check whether a second has elapsed since the last tick.  If so, adjust
   the timer and return 1; otherwise, return 0.  */
static int
has_second_elapsed(grub_uint64_t *saved_time)
{
    grub_uint64_t current_time;

    current_time = grub_get_time_ms();
    if (current_time - *saved_time >= 600)
    {
        *saved_time = current_time;
        return 1;
    }
    else
        return 0;
}

static struct grub_input_viewer *viewers;

void grub_input_register_viewer(struct grub_input_viewer *viewer)
{
    viewer->next = viewers;
    viewers = viewer;
}

static grub_err_t
run_input(int nested)
{
    int i = 0;

reinit:

    g_current_input_text_index = 0;

    g_current_input_label_info[0] = 0;
    g_current_input_label_error[0] = 0;

    for (i = 0; i < MAX_INPUT_NUM; i++)
    {
        g_current_input_text[i][0] = '\0'; //清空
    }

    s_saved_time = 0;

    input_init(nested);

    while (1)
    {
        int c;
        int input_len = 0;

        if (grub_normal_exit_level)
            return -1;

        c = grub_getkey_noblock();

        if (c != GRUB_TERM_NO_KEY)
        {
            switch (c)
            {
            case GRUB_TERM_CTRL | 'c':
            {
                grub_cmdline_run(1, 0);
                goto reinit;
            }
            case GRUB_TERM_CTRL | 'l':
            {
                //grub_cmdline_run(1, 0);
                struct grub_menu_entry e;
                grub_menu_entry_run(&e);
                goto reinit;
            }
            case GRUB_TERM_CTRL | 'd':
            {
                g_current_input_log[0] = 0;
            }
            case GRUB_TERM_KEY_UP:
            {
                if (g_current_input_text_index >= 1)
                {
                    g_current_input_text_index--;
                }
                else
                {
                    g_current_input_text_index = g_input_edit_num - 1;
                }
                g_input_show_cursor = 1;
                goto refresh;
            }
            case GRUB_TERM_TAB:
            case GRUB_TERM_KEY_DOWN:
            {
                if (g_current_input_text_index < g_input_edit_num - 1)
                {
                    g_current_input_text_index++;
                }
                else
                {
                    g_current_input_text_index = 0;
                }
                g_input_show_cursor = 1;
                goto refresh;
            }
            case '\r':
            {
                if (grub_strlen(g_current_input_text[g_current_input_text_index]) == 0)
                {
                    grub_snprintf(g_current_input_label_error, MAX_TEXT_INFO_LEN, "Password can not be null, Please input!");
                    goto check_failed;
                }
                if (!input_check_password)
                {
                    grub_snprintf(g_current_input_label_error, MAX_TEXT_INFO_LEN, "Can not verify password!");
                    goto check_failed;
                }

                // 1成功， 0失败
                if (1 == input_check_password(g_current_input_text[g_current_input_text_index], g_current_input_label_error, MAX_TEXT_INFO_LEN))
                {
                    // 启动系统
                    goto check_failed;
                }
                else
                {
                    goto check_failed;
                }
            }
            case '\b':
            {
                input_len = grub_strlen(g_current_input_text[g_current_input_text_index]);
                if (input_len > 0)
                {
                    input_len--;
                }
                g_current_input_text[g_current_input_text_index][input_len] = '\0';
                goto refresh;
            }
            default:
            {
                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
                {
                    input_len = grub_strlen(g_current_input_text[g_current_input_text_index]);
                    if (input_len < MAX_INPUT_TEXT_LEN)
                    {
                        g_current_input_text[g_current_input_text_index][input_len] = c;
                        g_current_input_text[g_current_input_text_index][input_len + 1] = '\0';

                        g_current_input_label_info[0] = 0;
                        g_current_input_label_error[0] = 0;

                        goto refresh;
                    }
                }
            }
            break;
            }
        }
        else
        {
            if (has_second_elapsed(&s_saved_time))
            {
                if (g_input_show_cursor == 0)
                {
                    g_input_show_cursor = 1;
                }
                else
                {
                    g_input_show_cursor = 0;
                }
                goto refresh;
            }
        }
        continue;

    check_failed:
        g_current_input_text_index = 0;
        for (i = 0; i < MAX_INPUT_NUM; i++)
        {
            g_current_input_text[i][0] = '\0'; //清空
        }
        s_saved_time = 0;

    refresh:
        viewers->redraw_input(viewers->data);
    }

    /* Never reach here.  */
}

static grub_err_t
show_input(int nested)
{
    while (1)
    {
        if (GRUB_ERR_NONE == run_input(nested))
        {
            break;
        }
    }

    return GRUB_ERR_NONE;
}

grub_err_t
grub_show_input(int nested)
{
    grub_err_t err1, err2;

    while (1)
    {
        err1 = show_input(nested);
        grub_print_error();

        if (grub_normal_exit_level)
            break;

        err2 = grub_auth_check_authentication(NULL);
        if (err2)
        {
            grub_print_error();
            grub_errno = GRUB_ERR_NONE;
            continue;
        }

        break;
    }

    return err1;
}
