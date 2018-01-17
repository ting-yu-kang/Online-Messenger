#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>

#define ESC_CUU	"\033[A"
#define ESC_CUD	"\033[B"
#define ESC_CUF	"\033[C"
#define ESC_CUB	"\033[D"
#define ESC_CNL	"\033[E"
#define ESC_CUP	"\033[H"
#define ESC_ED	"\033[J"
#define ESC_SCP	"\033[s"
#define ESC_RCP	"\033[u"
#define ESC_HIDE	"\033[?25l"
#define ESC_SHOW	"\033[?25h"

#define ESC_RESET	"\033[0m"
#define ESC_BLACK	"\033[30m"
#define ESC_RED		"\033[31m"
#define ESC_GREEN	"\033[32m"
#define ESC_YELLOW	"\033[33m"
#define ESC_BLUE	"\033[34m"
#define ESC_MAGENTA	"\033[35m"
#define ESC_CYAN	"\033[36m"
#define ESC_WHITE	"\033[37m"

#define LINE_50	"=================================================="

#define KEY_UP		200
#define KEY_DOWN	201
#define KEY_LEFT	202
#define KEY_RIGHT	203
#define KEY_F1		204
#define KEY_F2		205
#define KEY_F3		206
#define KEY_F4		207
#define KEY_BS		127
#define KEY_ENTER	10
#define KEY_SPACE	32
#define KEY_ESC		27


extern struct termios oios, uios;


void hscanf(const char* print_str, char* scan_str);
void hprintf(const char* print_str, int row, int col, const char* color_str);
void print_start();
void print_login(char* user_id, char* user_pw);
void print_text(const char* recv_id);
void print_long();
void print_file();
void print_files();
int scan_text(int& length, char* buffer);

void interface_init();
int interface_getc(int usec);
