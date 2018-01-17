#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#define PORT_NUM 19955

#define MAX_USER 16
#define MAX_DATA 1364
#define PACKET_FILE_SIZE 1319
#define FILE_BUFFER_SIZE 32768
#define FILE_NAME_SIZE 32
#define MAX_FILE_COUNT 50

#define STATE_START 0
#define STATE_REG	1
#define STATE_LOGIN	2
#define STATE_LIST	3
#define STATE_LOAD	4
#define STATE_TEXT	5
#define STATE_LONG	6
#define STATE_FILE	7
#define	STATE_FILES	8

#define PACK_TYPE_ERROR	0
#define PACK_TYPE_REG	1
#define PACK_TYPE_LOGIN	2
#define PACK_TYPE_TEXT	4
#define PACK_TYPE_FILE	5
#define PACK_TYPE_RECV	6
#define PACK_TYPE_INFO	7
#define PACK_TYPE_DATA	8
#define PACK_TYPE_CLIENT_CLOSE	9

#define ERROR_EXIST_ID		1
#define ERROR_WRONG_PW		2
#define ERROR_EMPTY_MESSQ	3

struct packet {
	int type;
	int error;
	int self_id;
	int send_id, recv_id;
	int data_length;
	
	char user_id[30], user_pw[30];
	char data[MAX_DATA];
	//for_file
//	char filename[FILE_NAME_SIZE+1];
//	int offset;
//	bool end_of_file;
//	int file_part_size;
//	char file_part[PACKET_FILE_SIZE+1];
};

struct User {
	char id[30], pw[30];
};

struct List {
	int num_user;
	char id[MAX_USER][30];
	int status[MAX_USER];
};

struct File {
	char eof;
	int size;
	int offset;
	char name[32];
	char data[PACKET_FILE_SIZE+4];
};

int set_fd(int num_fd, int* fd, fd_set& fds) {
	int maxfd = 0;

	for (int i = 0; i < num_fd; i++) {
		maxfd = maxfd > fd[i] ? maxfd : fd[i];
		FD_SET(fd[i], &fds);
	}

	return maxfd;
}

char* scramble(char* str, int n) {
	char* start = str;

	str[--n] = '\0';
	while (n > 0)
		str[--n] = rand() % 95 + 32;

	return start;
}

unsigned char AES_key[] = {"DoReMiFaDoReMiFa"};