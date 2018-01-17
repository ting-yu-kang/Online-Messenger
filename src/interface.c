#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include "interface.h"

struct termios oios, uios, eios;
int row, col;

void hscanf(const char* print_str, char* scan_str) {
}
void hprintf(const char* print_str, int row, int col, const char* color_str) {
	printf("\033[%d;%dH%s%s", row, col, color_str, print_str);
}
void print_start() {
	// Erase from 1 ~ 10
	printf(ESC_CUP ESC_ED ESC_HIDE);

	// Move Cursor to (10, 15) and Move Down 4
	hprintf(LINE_50, 10, 15, ESC_RESET);
	hprintf("NEW", 15, 27, ESC_RED);
	hprintf("OLD", 15, 52, ESC_RED);
	hprintf(LINE_50, 20, 15, ESC_RESET);
	//int a;
	//scanf("%d",&a);
	printf("\n");
}
void print_login(char* user_id, char* user_pw) {
	if (tcsetattr(STDIN_FILENO, TCSANOW,  &oios) < 0) {
		perror("tcsetattr() ICANON");
		exit(1);
	}

	printf(ESC_CUP ESC_ED ESC_HIDE);

	hprintf(LINE_50, 10, 15, ESC_RESET);
	hprintf(LINE_50, 15, 15, ESC_RESET);
	hprintf("ID: ", 12, 27, ESC_RESET);
	scanf("%s", user_id);
	hprintf("Password: ", 12, 27, ESC_RESET);
	scanf("%s", user_pw);

//	if (tcsetattr(STDIN_FILENO, TCSANOW,  &uios) < 0) {
//		perror("tcsetattr() ICANON");
//		exit(1);
//	}
}
void print_text(const char* recv_id) {
	printf(ESC_SCP);
	hprintf(LINE_50, 1, 15, ESC_RESET);
	hprintf(LINE_50, 31, 15, ESC_RESET);
	hprintf("To: ", 2, 16, ESC_RESET);
	printf("%s", recv_id);
	hprintf("Text Mode\033[0K", 3, 16, ESC_RESET);
	printf(ESC_RCP);
	
	fflush(stdout);

	if (tcsetattr(STDIN_FILENO, TCSANOW,  &eios) < 0) {
		perror("tcsetattr() ICANON");
		exit(1);
	}
}
void print_long() {
	printf(ESC_SCP);
	hprintf("Long Text Mode\033[0K", 3, 16, ESC_RESET);
	printf(ESC_RCP);

	fflush(stdout);
}
void print_file() {
	printf(ESC_SCP);
	hprintf("File Mode\033[0K", 3, 16, ESC_RESET);
	printf(ESC_RCP);

	fflush(stdout);
}
void print_files() {
	printf(ESC_SCP);
	hprintf("Multiple File Mode\033[0K", 3, 16, ESC_RESET);
	printf(ESC_RCP);

	fflush(stdout);
}

int scan_text(int& pos, char* buffer) {
	int c = 0;

	if ((c = interface_getc(1)) > 0) {
		if (c == KEY_BS) {
			if (pos > 0) {
				if (buffer[pos - 1] != KEY_ENTER) {
					buffer[pos--] = 0;
					printf("\b\b\b\033[0K");
				}
				else {
					printf("\b\b\033[0K");
				}
			}
			else {
				printf("\b\b\033[0K");
			}
		}
		else if (c == KEY_ENTER) {
			if (pos > 0) {
				buffer[pos++] = c;
			}
			printf("\033[16G");
		}
		else if (c >= 32 && c <= 126) {
			buffer[pos++] = c;
		}
		else if (c >= KEY_F1 && c <= KEY_F4) {
			printf("\b\b\b\b\b\b\033[0K");
		}
	}

	fflush(stdout);

	return c;
}

/*
 * Setup termios environment for keyboard input command
 */
void interface_init() {
	if (tcgetattr(STDIN_FILENO, &oios) < 0) {
		perror("tcgetattr()");
		exit(1);
	}

	uios = oios;
//	uios.c_lflag &= ~ICANON|ECHO|ECHOE;
	uios.c_lflag &= ~(ICANON|ECHO|ECHOE);
	uios.c_cc[VMIN] = 1;
	uios.c_cc[VTIME] = 0;

	eios = oios;
	eios.c_lflag &= ~(ICANON|ECHOE)|ECHO;
	eios.c_cc[VMIN] = 1;
	eios.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSANOW,  &uios) < 0) {
		perror("tcsetattr() ICANON");
		exit(1);
	}
}
int interface_getc(int usec) {
	int key = 0;
	char c = 0;
	char buffer[10];

	bzero(buffer, sizeof(buffer));

	// 0.000001 sec
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = usec;

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	int response = 1;

	if (usec > 0)
		response = select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);

	if (response > 0) {
		int n = read(STDIN_FILENO, buffer, 10000);

		if (n == 1 && buffer[0] >= 32 && buffer[0] <= 126) {
			key = buffer[0];
		}
		else if (n == 1 && buffer[0] == KEY_BS) {
			key = buffer[0];
		}
		else if (n == 1 && buffer[0] == KEY_ENTER) {
			key = buffer[0];
		}
		else if (n == 1 && buffer[0] == KEY_ESC) {
			key = buffer[0];
		}
		else if (n == 3 && buffer[0] == 27 && buffer[1] == 91) {
			if (buffer[2] == 65)
				key = KEY_UP;
			else if (buffer[2] == 66)
				key = KEY_DOWN;
			else if (buffer[2] == 68)
				key = KEY_LEFT;
			else if (buffer[2] == 67)
				key = KEY_RIGHT;
		}
		else if (n == 5 && buffer[0] == 27 && buffer[1] == 91 && buffer[4] == 126) {
			if (buffer[2] == 49 && buffer[3] == 49)
				key = KEY_F1;
			else if (buffer[2] == 49 && buffer[3] == 50)
				key = KEY_F2;
			else if (buffer[2] == 49 && buffer[3] == 51)
				key = KEY_F3;
			else if (buffer[2] == 49 && buffer[3] == 52)
				key = KEY_F4;
		}
	}

	return key;
}


/*	int length = 0;
	char buffer[1000];

	bzero(buffer, sizeof(buffer));

	while(1) {
		char c;

		read(STDIN_FILENO, &c, 1);
		
		buffer[length] = c;
		length++;

		if (c == 'q') break;
	}

	printf("%s\n", buffer);

	int i = 0;
	for (i = 0; i < length; i++)
		printf("%4d", buffer[i]);

	if (tcsetattr(STDIN_FILENO, TCSADRAIN, &uios) < 0) {
		perror("tcsetattr() ~ICANON");
		exit(1);
	}

	return;
}*/
