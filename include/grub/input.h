#ifndef GRUB_INPUT_HEADER
#define GRUB_INPUT_HEADER 1

#define MAX_INPUT_TEXT_LEN 50
#define MAX_INPUT_NUM 5
#define MAX_TEXT_INFO_LEN 256

extern char g_current_input_text[MAX_INPUT_NUM][MAX_INPUT_TEXT_LEN + 1];
extern int g_current_input_text_index;
extern char g_current_input_label_info[256];
extern char g_current_input_label_error[256];
extern long g_input_edit_num;
extern int g_input_show_cursor;

extern char g_current_input_log[1024*1024];
extern void
input_log(const char *fmt, ...);

extern int (*input_check_password)(const char *password, char *error, int len);

#endif //GRUB_INPUT_HEADER