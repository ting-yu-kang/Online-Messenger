#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include "connect.h"
#include "interface.h"
#include "stdafx.h"
#include "AES.h"

int sockfd;
struct sockaddr_in servaddr;
struct packet sendpack, recvpack;
int enc_size = 16*(sizeof(sendpack)/16);
char enc_buffer[sizeof(sendpack)];
int login = 0, state = 0;
char *IP;
AES aes(AES_key);

void write_encrypt(){
	memcpy(enc_buffer, &sendpack, sizeof(sendpack));
	//for(int j=0; j<enc_size; j++)printf("%X ",(unsigned char)enc_buffer[j]);
	aes.Cipher((void *)&enc_buffer, enc_size);
	//printf("~~\n");
	//for(int j=0; j<enc_size; j++)printf("%X ",(unsigned char)enc_buffer[j]);
	memcpy(&sendpack, enc_buffer, sizeof(sendpack));
	write(sockfd, &sendpack, sizeof(sendpack));
}

int read_decrypt(){
	int n = read(sockfd, &recvpack, sizeof(struct packet));
	memcpy(enc_buffer, &recvpack, sizeof(recvpack));
	//for(int j=0; j<enc_size; j++)printf("%X ",(unsigned char)enc_buffer[j]);
	aes.InvCipher((void *)&enc_buffer, enc_size);
	//for(int j=0; j<enc_size; j++)printf("%X ",(unsigned char)enc_buffer[j]);
	memcpy(&recvpack, enc_buffer, sizeof(recvpack));
	return n;
}

void while_loop(){
	// Init Interface I/O
	interface_init();
	login = 0, 
	state = 0;
	int load = 0;
	int listed = 0;
	int select = -1;
	int quit = 0;
	
	// Main Entry
	int key = 0;
	int self_id;
	char user_id[30], user_pw[30];
	struct List list;

	int length = 0;
	char buffer[MAX_DATA];
	char file_buffer[PACKET_FILE_SIZE];

	char filename[MAX_FILE_COUNT][FILE_NAME_SIZE];
	char tmpfilename[MAX_FILE_COUNT][FILE_NAME_SIZE];
	char tmpfilecount = 0;
	char filebuff[MAX_FILE_COUNT][FILE_BUFFER_SIZE];
	int filesize[MAX_FILE_COUNT];
	int file_cut[MAX_FILE_COUNT];
	int file_dest[MAX_FILE_COUNT];
	int file_sending[MAX_FILE_COUNT];
	bzero(file_sending, sizeof(file_sending));
	int lastfileturn =-1, fileturn = 0, num_file_trans = 0;
	
	while (!quit) {
		// Initial sendpack, recvpack
		bzero(&sendpack, sizeof(sendpack));
		bzero(&recvpack, sizeof(recvpack));

		// State Decision
		if (!login) {
			if (key == KEY_LEFT)
				state = STATE_REG;
			else if (key == KEY_RIGHT)
				state = STATE_LOGIN;
		}
		else if (login) {
			if (!load)
				state = STATE_LOAD;
			else if (select < 0)
				state = STATE_LIST;
		}

		// Receiving Transmission
		if (login && load && listed) {
			sendpack.type = PACK_TYPE_RECV;
			write_encrypt();

			read_decrypt();
			
			//receive file
			if(recvpack.type == PACK_TYPE_FILE) {
				struct File file;

				memcpy(&file, recvpack.data, sizeof(struct File));
				char name[FILE_NAME_SIZE];
				sprintf(name, "./download/%s", file.name);

				printf("filename = %s\n", name);
				printf("offset = %d\n", file.offset);

				int fd;
				if(file.offset == 0)
					fd = open(name, O_WRONLY|O_CREAT|O_TRUNC, 0666);
				else
					fd = open(name, O_WRONLY|O_APPEND, 0666);

				if (fd < 0) {
					perror("open()");
					exit(1);
				}

				printf("%10s->%10s\n", list.id[recvpack.send_id], list.id[recvpack.recv_id]);
				printf("receive data part:%d\n", file.offset);

				write(fd, file.data, file.size);

				if(file.eof) {
					printf("receive file complete: %s\n", name);
					printf("total file size: %d\n", lseek(fd, 0, SEEK_END));
				}

				close(fd);
			}
			else if(recvpack.type == PACK_TYPE_TEXT) {
				printf("%10s->%10s\n", list.id[recvpack.send_id], list.id[recvpack.recv_id]);
				printf("Receive message: \n%s\n", recvpack.data);
			}
		}

		bzero(&sendpack, sizeof(sendpack));
		bzero(&recvpack, sizeof(recvpack));

		// User Control
		switch(state) {
			case STATE_START: {
				print_start();

				key = interface_getc(0);
//				printf("key = %d\n", key);
			break;
			}
			case STATE_REG: {
				int exist = 1;

				while (exist) {
					// Type ID, Password
					bzero(user_id, 30);
					bzero(user_pw, 30);
    
					print_login(user_id, user_pw);
    
					sendpack.type = PACK_TYPE_REG;
					strcpy(sendpack.user_id, user_id);
					strcpy(sendpack.user_pw, user_pw);
    
					// Send New ID, Password to Server Side
					write_encrypt();

					// Receive Response from Server
					read_decrypt();

					// No Error Occurred
					if (recvpack.type != PACK_TYPE_ERROR) {
						self_id = recvpack.self_id;
						exist = 0;
						login = 1;

						state = STATE_LOAD;

						printf("Login\n");
					}
					// User ID already exist
					else if (recvpack.error == ERROR_EXIST_ID) {
						bzero(&sendpack, sizeof(sendpack));
						bzero(&recvpack, sizeof(recvpack));

						printf("\"%s\" already exist!\n", user_id);
					}
				}
			break;
			}
			case STATE_LOGIN: {
				int correct = 0;

				while(!correct) {
					// Type ID, Password
					bzero(user_id, 30);
					bzero(user_pw, 30);
   
					print_login(user_id, user_pw);
                
					// Send ID, Password to Server Side
					sendpack.type = PACK_TYPE_LOGIN;
					strcpy(sendpack.user_id, user_id);
					strcpy(sendpack.user_pw, user_pw);
					sendpack.data_length = MAX_DATA;
					scramble(sendpack.data, MAX_DATA);
					write_encrypt();

					// Receive Response from Server
					int n = read_decrypt();
					printf("Receive %d type %d Byte packet\n", recvpack.type, n);
                
					// No Error Occurred
					if (recvpack.type != PACK_TYPE_ERROR) {
						self_id = recvpack.self_id;
						correct = 1;
						login = 1;
                
						state = STATE_LOAD;
                
						printf("Login\n");
					}
					// Type wrong password
					else if (recvpack.error == ERROR_WRONG_PW) {
						bzero(&sendpack, sizeof(sendpack));
						bzero(&recvpack, sizeof(recvpack));

						printf("Wrong Password!\n");
						sleep(1);
					}
				}
			break;
			}
			case STATE_LOAD: {
				sendpack.type = PACK_TYPE_INFO;

				write_encrypt();
			
				recvpack.data_length = 314513;
				read_decrypt();

				if (recvpack.type != PACK_TYPE_ERROR) {
					bzero(&list, sizeof(list));
					memcpy(&list, &recvpack.data, sizeof(list));
				
					printf("list num_user = %d\n", list.num_user);
					for (int i = 0 ; i < list.num_user; i++) {
						printf("%d: ID = %s, status = %d\n", i,list.id[i], list.status[i]);
					}
					printf("state list, select someone\n");	
					load = 1;
				}
			break;
			}
			case STATE_LIST: {
				// Show Friend List
				// Friend to send
				scanf("%d", &select);	
				printf("select = %d\n", select);
				printf(ESC_CUP ESC_ED ESC_SHOW);
				hprintf("", 4, 16, ESC_RESET);
				print_text(list.id[select]);
				listed = 1;
				state = STATE_TEXT;
				
			break;
			}
			case STATE_TEXT: {
				int c = scan_text(length, buffer);
//				printf("%d: %s\n", length, buffer);
				if (c == KEY_ENTER) {		
					sendpack.type = PACK_TYPE_TEXT;
					sendpack.send_id = self_id;
					sendpack.recv_id = select;
					sendpack.data_length = length;
					strcpy(sendpack.data, buffer);
					sendpack.data[length - 1] = 0;
					// Send Text to Server
					write_encrypt();

					// Receive Response from Server
					read_decrypt();

					// Prepare sending next line
					length = 0;
					bzero(buffer, sizeof(buffer));
					sendpack.data_length = 0;
					bzero(sendpack.data, sizeof(sendpack.data));

					printf(ESC_CNL "\033[16K");
				}
				else if (c == KEY_F2) {
					print_long();

					state = STATE_LONG;
				}
				else if (c == KEY_F3) {
					print_file();

					state = STATE_FILE;
				}
				else if (c == KEY_ESC){
					load = 0;
					listed = 0;
					select = -1;
				}
//					quit = 1;
			break;
			}
			case STATE_LONG: {
				int c = scan_text(length, buffer);

				if (c == KEY_F1) {
					print_text(list.id[select]);

					state = STATE_TEXT;
				}
				else if (c == KEY_F3) {
					print_file();

					state = STATE_FILE;
				}
				else if (c == KEY_ESC){
					load = 0;
					listed = 0;
					select = -1;
				}
			break;
			}
			case STATE_FILE: {
				int c = scan_text(length, buffer);
				if (c == KEY_SPACE){
					buffer[length - 1] = 0;
					strcpy(tmpfilename[tmpfilecount], buffer);
					tmpfilecount++;
					length = 0;
					bzero(buffer, sizeof(buffer));
				}
				if (c == KEY_ENTER) {
					buffer[length - 1] = 0;
					strcpy(tmpfilename[tmpfilecount], buffer);
					tmpfilecount++;
					char name[FILE_NAME_SIZE];
					for(int k = 0 ; k < tmpfilecount ; k ++){
						for(int i = 0 ; i < MAX_FILE_COUNT ; i ++){
							if(file_sending[i]==0){
								fileturn = i;
								break;
							}
						}
						strcpy(filename[fileturn], tmpfilename[k]);
						sprintf(name, "./upload/%s", tmpfilename[k]);
						int fd = open(name, O_RDONLY, 0666);
						filesize[fileturn] = read(fd, filebuff[fileturn], FILE_BUFFER_SIZE);
						close(fd);
						file_cut[fileturn] = 0;
						file_dest[fileturn] = select;
						file_sending[fileturn]=1;
						num_file_trans++;
					}
					length = 0;
					tmpfilecount = 0;
					bzero(buffer, sizeof(buffer));
				}
				else if (c == KEY_F1) {
					print_text(list.id[select]);

					state = STATE_TEXT;
				}
				else if (c == KEY_F2) {
					print_long();

					state = STATE_LONG;
				}
				else if (c == KEY_ESC){
					load = 0;
					listed = 0;
					select = -1;
				}
			break;
			}
			default:
			break;
		}

		bzero(&recvpack, sizeof(recvpack));
		bzero(&sendpack, sizeof(sendpack));

		// File Transfer
		if (num_file_trans > 0) {
			for(int i = lastfileturn + 1 ; ; i++){
				i%=MAX_FILE_COUNT;
				if(file_sending[i]==1){
					fileturn = i;
					lastfileturn = fileturn;
					break;
				}
			}

			sendpack.type = PACK_TYPE_FILE;
			sendpack.send_id = self_id;
			sendpack.recv_id = select;
			struct File file;
			file.offset = file_cut[fileturn];
			if (filesize[fileturn] - file_cut[fileturn] <= PACKET_FILE_SIZE) {
				file.eof = 1;
				file.size = filesize[fileturn] - file_cut[fileturn];
				file_sending[fileturn]=0;
				num_file_trans--;
			}
			else {
				file.eof = 0;
				file.size = PACKET_FILE_SIZE;
				file_cut[fileturn] += PACKET_FILE_SIZE;
			}
			strcpy(file.name, filename[fileturn]);
			memcpy(file.data, filebuff[fileturn]+file.offset, file.size);
			memcpy(sendpack.data, &file, sizeof(file));
			// Send File packet to Server
			write_encrypt();

			// Receive response
			read_decrypt();

		}
	}
	return;
}

void create_socket(){
	sockfd=socket(AF_INET,SOCK_STREAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(IP);
	servaddr.sin_port=htons(PORT_NUM);
	if (inet_aton(IP, &servaddr.sin_addr) == 0) {
		perror("inet_aton()");
		exit(1);
	}

	// Connect to Server
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect()");
		exit(1);
	}
	printf("connection complete!\n");
	while_loop();
}

void sigint_shut_down(int signo) {
	if(login==1){
	char MSG1[50]="log out!\n";
	write(STDERR_FILENO,MSG1,sizeof(MSG1)); 
	sendpack.type = PACK_TYPE_CLIENT_CLOSE;
	write_encrypt();
	close(sockfd);
	}
	else{
		char MSG3[50]="not login exit\n";
		write(STDERR_FILENO,MSG3,sizeof(MSG3)); 
		close(sockfd);
	}
	exit(0);
}

void sigpipe_shut_down(int signo) {
	char MSG[50]="server is shut down!\n";
	write(STDERR_FILENO,MSG,sizeof(MSG)); 
	close(sockfd);
	char input[3];
	login = -1;
	printf("Do you want to reconnect? (Y/N)\n");
	scanf("%s", input);
	fflush(stdout);
	if((strcmp(input, "y") == 0) || (strcmp(input, "Y") == 0)) {
		create_socket();
		while_loop();
	}
	else
		exit(0);
}

int main(int argc, char**argv) {
	srand(time(NULL));
	// SIGINT Handle
	struct sigaction act1,act2; 
	act1.sa_handler=sigint_shut_down;
	sigemptyset(&act1.sa_mask); 
	sigaddset(&act1.sa_mask,SIGINT);
	act1.sa_flags=0; 
	if(sigaction(SIGINT,&act1,NULL)<0) 
	{ 
		fprintf(stderr,"Install Signal Action1 Error:%s\n\a",strerror(errno)); 
		exit(1); 
	} 
	
	// SIGPIPE Handle
	act2.sa_handler=sigpipe_shut_down; 
	sigemptyset(&act2.sa_mask); 
	//sigaddset(&act2.sa_mask,SIGPIPE);
	sigaddset(&act2.sa_mask,SIGINT);
	act2.sa_flags=0; 
	if(sigaction(SIGPIPE,&act2,NULL)<0) 
	{ 
		fprintf(stderr,"Install Signal Action2 Error:%s\n\a",strerror(errno)); 
		exit(1); 
	} 


	// Socket Initial

	if (argc < 2) {
		printf("usage: client <IP address>\n");
		exit(1);
	}
	IP = new char[strlen(argv[1])+1];
	strcpy(IP, argv[1]);
	
	create_socket();
	
	return 0;
}
