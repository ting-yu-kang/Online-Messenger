#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <queue>
#include "connect.h"
#include "stdafx.h"
#include "AES.h"

using namespace std;

queue<struct packet>* MESSQ = new queue<struct packet>[MAX_USER]();
struct packet recvpack, sendpack;
int enc_size = 16*(sizeof(recvpack)/16);
char enc_buffer[sizeof(recvpack)];
AES aes(AES_key);

void user_init(int fd, int& num_user, struct User* user) {
	int n = read(fd, &num_user, sizeof(int));

	if (n < 0) {
		perror("read()");
		exit(1);
	}
	else if (n == 0) {
		num_user = 1;

		strcpy(user->id, "Defaulter");
		strcpy(user->pw, "123456789");

		write(fd, &num_user, sizeof(int));
		write(fd, user, sizeof(struct User));
	}
	else {
		printf("User Total: %d\n", num_user);

		for (int i = 0; i < num_user; i++) {
			read(fd, &user[i], sizeof(struct User));
			printf("ID[%d]: %-30s| %s\n", i, user[i].id, user[i].pw);
		}
	}


	int Mfd = open("./doc/messageQ", O_RDONLY, 0666);
	if(Mfd > 0) {
		for(int i = 0; i < MAX_USER; i++) {
			int size;
			read(Mfd,&size, sizeof(int));
			size = (int)size;
			if(size > 0) {
				printf("size = %d\n", size);
				for(int j = 0; j < size; j++) {
					struct packet tmp;
					read(Mfd, &tmp, sizeof(struct packet));
					MESSQ[i].push(tmp);
					printf("Restore: %s->%s%30s\n", user[tmp.send_id].id, user[tmp.recv_id].id, tmp.data);
				}
			}
		}
	}
	close(Mfd);
}

void shut_down(int signo) {
	printf("\n\n\n\n\n\n");
	int fd = open("./doc/messageQ", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	printf("write messageQ to file..\n");
	for(int i = 0; i < MAX_USER; i++) {
		int size = (int)(MESSQ[i].size());
		write(fd, &size, sizeof(int));
		if(size > 0) {
			printf("size = %d\n", size);
			struct packet tmp;
			while(!MESSQ[i].empty()) {
				tmp = MESSQ[i].front();
				MESSQ[i].pop();
				write(fd, &tmp, sizeof(struct packet));
					printf("Backup: %d->%d%30s\n", tmp.send_id, tmp.recv_id, tmp.data);
			}
		}
	}
	close(fd);
	char MSG[50]="shut down in 1 sec...\n";
	write(STDERR_FILENO,MSG,sizeof(MSG));
	sleep(1);
	exit(0);
}



void set_fl(int fd, int flags){
	int val;
    if((val = fcntl(fd, F_GETFL, 0))< 0)
        perror("fcntli get");
   
    val |= flags;
    if(fcntl(fd, F_SETFL, val)< 0)
        perror("fcntl set");
}
void clr_fl(int fd, int flags){
	int val;
    if ( (val = fcntl(fd, F_GETFL, 0)) < 0)
        perror("fcntl get");
    
    val &= ~flags;
    
    if (fcntl(fd, F_SETFL, val) < 0)
        perror("fcntl set");
}

void write_encrypt(int sockid){
	memcpy(enc_buffer, &sendpack, sizeof(sendpack));
	//for(int j=0; j<enc_size; j++)printf("%X ",(unsigned char)enc_buffer[j]);
	aes.Cipher((void *)&enc_buffer, enc_size);
	//printf("~~\n");
	//for(int j=0; j<enc_size; j++)printf("%X ",(unsigned char)enc_buffer[j]);
	memcpy(&sendpack, enc_buffer, sizeof(sendpack));
	write(sockid, &sendpack, sizeof(sendpack));
}

int read_decrypt(int sockid){
	int n = read(sockid, &recvpack, sizeof(struct packet));
	memcpy(enc_buffer, &recvpack, sizeof(recvpack));
	//for(int j=0; j<enc_size; j++)printf("%X ",(unsigned char)enc_buffer[j]);
	aes.InvCipher((void *)&enc_buffer, enc_size);
	//for(int j=0; j<enc_size; j++)printf("%X ",(unsigned char)enc_buffer[j]);
	memcpy(&recvpack, enc_buffer, sizeof(recvpack));
	return n;
}

int main(int argc, char**argv) {
	int listenfd,n;
	struct sockaddr_in servaddr,cliaddr;
	socklen_t clilen;
	
	listenfd=socket(AF_INET,SOCK_STREAM|O_NONBLOCK,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(PORT_NUM);
	bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

	listen(listenfd,1024);

	//User Data
	int user_fd = open("./doc/user_data", O_RDWR|O_CREAT, 0666);

	if (user_fd < 0) {
		perror("open()");
		exit(1);
	}

	// Initial User Data
	int num_user = 0;
	struct User* user = new struct User[MAX_USER];
	user_init(user_fd, num_user, user);

	// Block & Non-blocking
	fd_set rfds;

	int num_conn = 0;
	int* connfd = new int[MAX_USER]();
	int* sockfd = new int[MAX_USER]();

	// Time Value
	struct timeval th, tz;
	th.tv_sec = 0;
	th.tv_usec = 0;
	tz.tv_sec = 0;
	tz.tv_usec = 0;

	// SIGINT Handle
	struct sigaction act; 
	act.sa_handler=shut_down; 
	sigemptyset(&act.sa_mask); 
	sigaddset(&act.sa_mask,SIGINT);
	act.sa_flags=0; 
	if(sigaction(SIGINT,&act,NULL)<0) { 
		fprintf(stderr,"Install Signal Action Error:%s\n\a",strerror(errno)); 
		exit(1); 
	} 

	// Server Listening
	while(1) {
		clilen = sizeof(cliaddr);
		if ((connfd[num_conn] = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen)) > 0) {
			printf("connfd[%d] = %d\n", num_conn, connfd[num_conn]);
			num_conn++;
		}

		// Select()
		FD_ZERO(&rfds);
		int fd1 = set_fd(num_conn, connfd, rfds);
		int fd2 = set_fd(num_user, sockfd, rfds);
		int maxfd = fd1 > fd2 ? fd1 : fd2;
		int num_response = select(maxfd + 1, &rfds, NULL, NULL, &th);

		if (num_response == 0)
			continue;

//		printf("number response = %d\n", num_response);

		// For sockets without user registered yet
		for (int i = 0; i < num_conn; i++) {
			if (FD_ISSET(connfd[i], &rfds)) {
				bzero(&recvpack, sizeof(recvpack));
				bzero(&sendpack, sizeof(sendpack));
				int n = read_decrypt(connfd[i]);
				printf("Receive %d type %d Byte packet\n", recvpack.type, n);

				int packtype = recvpack.type;

				switch(packtype) {
					case PACK_TYPE_REG: {
						int exist = 0;

						// If User ID already exist
						for (int j = 0; j < num_user; j++) {
							printf("compare %s, %s\n", recvpack.user_id, user[j].id);
        
							if(strcmp(recvpack.user_id, user[j].id) == 0) {
								exist = 1;
								break;
							}
						}
        
						if (exist) {
							sendpack.type = PACK_TYPE_ERROR;
							sendpack.error = ERROR_EXIST_ID;

							write_encrypt(connfd[i]);
        
							printf("ID \"%s\" exist\n", recvpack.user_id);
						}
						else {
							strcpy(user[num_user].id,recvpack.user_id);
							strcpy(user[num_user].pw,recvpack.user_pw);
							
							
							lseek(user_fd, 0, SEEK_END);
							write(user_fd, user+num_user, sizeof(struct User));
							num_user++;
							lseek(user_fd, 0, SEEK_SET);
							write(user_fd, &num_user, sizeof(int));
							
							sendpack.type = recvpack.type;
							sendpack.self_id = num_user - 1;

							write_encrypt(connfd[i]);

							// connfd -> sockfd
							sockfd[num_user - 1] = connfd[i];
							connfd[i] = connfd[num_conn - 1];
							num_conn--;
							i--;

							printf("ID \"%s\" register login\n", recvpack.user_id);
						}
					break;
					}
	        		case PACK_TYPE_LOGIN: {
						int correct = -1;
        
						// If Password is correct
						for (int j = 0; j < num_user; j++) {
							if(strcmp(recvpack.user_id, user[j].id) == 0
							&& strcmp(recvpack.user_pw, user[j].pw) == 0) {
								correct = j;
								break;
							}
						}
        
						if (correct >= 0) {
							sendpack.type = recvpack.type;
							sendpack.self_id = correct;
        
							write_encrypt(connfd[i]);

							// connfd -> sockfd
							sockfd[correct] = connfd[i];
							connfd[i] = connfd[num_conn - 1];
							num_conn--;
							i--;

							printf("ID \"%s\" login\n", recvpack.user_id);
						}
						else {
							sendpack.type = PACK_TYPE_ERROR;
							sendpack.error = ERROR_WRONG_PW;
							
							write_encrypt(connfd[i]);
        
							printf("ID \"%s\" wrong password: %s\n", recvpack.user_id, recvpack.user_pw);
						}
					break;
					}
				}
			}
		}

		// For sockets with user registered
		for (int i = 0; i < num_user; i++) {
			if (FD_ISSET(sockfd[i], &rfds)) {
				bzero(&recvpack, sizeof(recvpack));
				bzero(&sendpack, sizeof(sendpack));
				read_decrypt(sockfd[i]);
				//read(sockfd[i], &recvpack, sizeof(struct packet));
				int packtype = recvpack.type;
				//if(packtype == 0)
				//	printf("Packet Recieved from user: %s, type:%d\n",user[i].id, packtype);

				switch(packtype) {
					case PACK_TYPE_RECV: {
						if (MESSQ[i].empty()) {
							sendpack.type = PACK_TYPE_ERROR;
							sendpack.error = ERROR_EMPTY_MESSQ;
						}
						else {
							sendpack = MESSQ[i].front();
							MESSQ[i].pop();
						}
						write_encrypt(sockfd[i]);
						
					break;
					}
					case PACK_TYPE_TEXT: {
						MESSQ[recvpack.recv_id].push(recvpack);
						printf("Log: %30s->%30s\n%s\n", user[recvpack.send_id].id, user[recvpack.recv_id].id, recvpack.data);
					printf("STATE LOGIN PACKET SENT\n");

						// No Error Occurred
						sendpack = recvpack;
						write_encrypt(sockfd[i]);
					break;
					}
					case PACK_TYPE_FILE: {
						MESSQ[recvpack.recv_id].push(recvpack);
						struct File file;
						memcpy(&file, recvpack.data, sizeof(struct File));
						printf("Log: %30s->%30s\n", user[recvpack.send_id].id, user[recvpack.recv_id].id);
						printf("File name: %s , File size = %d\n", file.name, file.size);

						// No Error Occurred
						sendpack = recvpack;
						write_encrypt(sockfd[i]);
					break;
					}
					case PACK_TYPE_INFO: {
						printf("Send User List\n");
						struct List list;
						list.num_user = num_user;
						for (int j = 0; j < num_user; j++) {
							strcpy(list.id[j], user[j].id);
							if (sockfd[j] > 0)
								list.status[j] = 1;
							else
								list.status[j] = 0;
						}

						sendpack.type = recvpack.type;
						sendpack.data_length = sizeof(list);
						memcpy(&sendpack.data, &list, sizeof(list));
						write_encrypt(sockfd[i]);
					break;
					}
					case PACK_TYPE_CLIENT_CLOSE: {
						sockfd[i] = 0;
						printf("User %d(%s) logged out\n", i, user[i].id);
					}
				}
			}
		}
//		close(connfd);
	}

	close(user_fd);
}
