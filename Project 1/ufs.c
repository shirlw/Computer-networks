#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#define RECV_BUFF_SIZE 100
#define SEND_BUFF_SIZE 1024
#define PORT_NO 8080
int main(int argc, char* argv[]){
	
	if (argc<2){
		fprintf(stderr, "You must provide a directory name\n");
		exit(1);
    }
	
	DIR *dp;
	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in sa; 
	char recvBuff[RECV_BUFF_SIZE];
	ssize_t recsize;
	socklen_t fromlen;
	char sendBuff[SEND_BUFF_SIZE];
	//int optval =1;
	//int numrv;

	memset(sendBuff, 0, sizeof(sendBuff));

	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(PORT_NO);
	fromlen = sizeof(sa);

	if (-1 == bind(sock, (struct sockaddr *)&sa, sizeof sa)) {
		perror("error bind failed");
		close(sock);
		return 1;
	}

	for (;;) {
		//setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		recsize = recvfrom(sock, (void*)recvBuff, sizeof recvBuff, 0, (struct sockaddr*)&sa, &fromlen);
		if (recsize < 0) {
			fprintf(stderr, "%s\n", strerror(errno));
			return 1;
		}
		printf("Requested filename: %.*s\n", (int)recsize, recvBuff);

		if((dp = opendir(argv[1])) == NULL) {
			fprintf(stderr,"cannot open directory: %s\n", argv[1]);
			exit(1);
		}
		/*go to the directory*/
		chdir(argv[1]);  
		/* Open the file that we wish to transfer */
		FILE *fp = fopen(recvBuff,"r");
		if(fp==NULL){
			fprintf(stderr,"Not Found\n");
			//strncpy(sendBuff, "Not Found", 10);
			sendBuff[SEND_BUFF_SIZE-1]='2';
			sendto(sock,sendBuff, SEND_BUFF_SIZE, 0,(struct sockaddr*)&sa, sizeof sa);
			return 1;
		}
		chdir("..");
	
		while (!feof(fp)){
			/*Read file in chunks of 1023 bytes. If read was success, send data. */
			int nread = fread(sendBuff,1,SEND_BUFF_SIZE-1,fp);
			printf("Bytes read %d \n", nread);
			sendBuff[SEND_BUFF_SIZE-1]='0';
			/*if it's the end of the file, put a 1 flag at the end*/
			if (feof(fp)){
				sendBuff[nread]='\0';
				sendBuff[SEND_BUFF_SIZE-1]='1';
				printf("End of file\n");
			}
			/*right now if the buffer is exactly filled with 1023 bytes, another empty chunk with 1 flag will still be sent*/
			printf("Sending the file ...\n");
			int n = sendto(sock,sendBuff, SEND_BUFF_SIZE, 0,(struct sockaddr*)&sa, sizeof sa);

			if(n<0) {
				perror("Problem sendto\n");
				exit(1);
			}
			if (ferror(fp)){
				printf("Error reading the file at server program\n");
				break;
			}
			usleep(1000);
			//memset(sendBuff, '0', sizeof(sendBuff));
		}
		fclose(fp);
	}
	close(sock);
	return 0;
}
