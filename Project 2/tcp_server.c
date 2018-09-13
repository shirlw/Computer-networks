#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include<string.h>
#include <unistd.h> //for write and read declaration (otherwise implicit declaration)
#include <arpa/inet.h>
#include "zlib.h"
#define CHUNK 16384
#include <assert.h>
#include <stdlib.h>

#define SEND_BUFF_SIZE 1024
#define RECV_BUFF_SIZE 1024

#define MAX_LINE_LEN 132

int numPlaces (int n) {
	if (n < 0) return 0;
	if (n < 10) return 1;
	if (n < 100) return 2;
	if (n < 1000) return 3;
	if (n < 10000) return 4;
	if (n < 100000) return 5;
	if (n < 1000000) return 6;
	if (n < 10000000) return 7;
	if (n < 100000000) return 8;
	if (n < 1000000000) return 9;
	return 10; //tell user to send a smaller file
}
 
int main(int argc,char* argv[]) {
	if (argc<3){
		fprintf(stderr, "You must provide an IP address and port number\n");
		return 1;
	}
	int listen_fd, comm_fd;
	int  bytesReceived = 0;
	struct sockaddr_in servaddr;
	char sendBuff[SEND_BUFF_SIZE];
	char recvBuff[RECV_BUFF_SIZE];
	char line[MAX_LINE_LEN];
	int linecnt=0;
	char* word;
	int file_size;
	int counter = 1;
	int filesize;
	int filebyte;
	int only8;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero( &servaddr, sizeof(servaddr));
 
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
	//servaddr.sin_addr.s_addr = htons(atoi(argv[1]));
	//servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	//servaddr.sin_port = htons(8080);
	servaddr.sin_port=htons(atoi(argv[2]));
 
    	bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));
 
	/*configure socket to reuse ports*/
	int optval = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	listen(listen_fd, 10);
 
	comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);
	
	/* Create file where data will be stored */
	FILE *fp;
	fp = fopen("received_file.zip", "w"); 
	if(NULL == fp){
        	printf("Error opening the file");
        	return 1;
	}
    
	do {
		/* Receive data in chunks of 1024  bytes */
		bytesReceived =recv(comm_fd,recvBuff,RECV_BUFF_SIZE,0);
		if(counter ==1 && bytesReceived <1024){
			printf("Number of bytes received: %d\n", bytesReceived);
			strncpy(sendBuff, recvBuff, 1);
			strncat(sendBuff, "/0", 1);
			sscanf(sendBuff, "%d", &filebyte);
			printf("file bytes is %d\n", filebyte);
			strncpy(sendBuff, &recvBuff[1], filebyte);
			sscanf(sendBuff, "%d", &filesize);
			printf("file size is %d\n", filesize);
			only8=0;
		}else if (counter ==1 && bytesReceived ==1024){
			printf("Number of bytes received: %d\n", bytesReceived);
			strncpy(sendBuff, recvBuff, 1);
			strncat(sendBuff, "/0", 1);
			sscanf(sendBuff, "%d", &filebyte);
			printf("file bytes is %d\n", filebyte);
			strncpy(sendBuff, &recvBuff[1], filebyte);
			sscanf(sendBuff, "%d", &filesize);
			printf("file size is %d\n", filesize);

			filesize = filesize - bytesReceived-(filebyte+1);
			if(fwrite(&recvBuff[filebyte+1],1,bytesReceived-(filebyte+1),fp)<0){
				printf("Error writing the file\n");
				return 1;
			}
			only8 = 1;
		}else{
			filesize = filesize - bytesReceived;
			printf("chunk %d\n", counter);
			if (bytesReceived<=0){
				printf("Recvfrom: Error in receiving the file\n");
				return 1;
			}else{   
				printf("Number of bytes received: %d\n", bytesReceived);
			}

			if(fwrite(recvBuff,1,bytesReceived,fp)<0){
				printf("Error writing the file\n");
				return 1;
			}
		}
		memset(recvBuff, '0', sizeof(recvBuff));
		counter++;
	}while(filesize>0);
	printf("End of file\n");
	if (only8 ==0){
		printf("There was only 8\n");	
	}else{
		printf("There was 1024\n");
	}
	fclose(fp);
	
	/*unzip the received zip file and get the name of the file in it*/
	system("unzip received_file.zip");
	system("unzip -l received_file.zip > del.txt");
	FILE *data_fp;
	data_fp = fopen("del.txt", "r"); 
	if(NULL == data_fp){
        	printf("Error opening the file");
        	return 1;
	}
	while (fgets(line,MAX_LINE_LEN, data_fp) !=NULL){
		linecnt++;
		if(linecnt<=3){
			continue;
		}
		word= strtok(line, " ");
		word= strtok(NULL, " ");
		word= strtok(NULL, " ");
		word= strtok(NULL, " ");
		break;
	}
	
	int len=strlen(word);
	if (word[len-1] == '\n'){
			word[len-1] = '\0';
	}
	
	fclose(data_fp);
	
	//open unzipped file and send it back to client
	FILE *fp2 = fopen(word,"r");
	if(fp2==NULL){
		fprintf(stderr,"Unzipped File Not Found\n");
		return 1;
	}

	//determine the requested file size
	fseek(fp2, 0L, SEEK_END);
	file_size = ftell(fp);
	fseek(fp2, 0L, SEEK_SET);
	
	//send the number of bytes in the file size, and the file size
	int filebytes = numPlaces(file_size);
	sprintf(sendBuff, "%d", filebytes);
	sprintf(recvBuff, "%d", file_size);
	strncat(sendBuff, recvBuff, filebytes);
	int n = send(comm_fd,sendBuff, strlen(sendBuff), 0);
	if(n<0) {
		perror("Problem sendto\n");
		return 1;
	}
	if (ferror(fp)){
		printf("Error reading the file at server program\n");
		return 1;
	}
	counter = 1;

	while (!feof(fp)){
		/*Read file in chunks of 100 bytes. If read was success, send data. */
		int nread = fread(sendBuff,1,SEND_BUFF_SIZE,fp);
		printf("Bytes read %d \n", nread);
		/*if it's the end of the file, put a 1 flag at the end*/
		if (feof(fp)){
			printf("End of file\n");
			int n = send(comm_fd,sendBuff, nread, 0);
			printf("Sending the file ...\n");
			printf("chunk %d\n", counter);
			counter++;
			if(n<0) {
				perror("Problem sendto\n");
				return 1;
			}
			if (ferror(fp)){
				printf("Error reading the file at server program\n");
				break;
			}
			break;
		}
		int n = send(comm_fd,sendBuff, SEND_BUFF_SIZE, 0);
		printf("Sending the file ...\n");
		printf("chunk %d\n", counter);
		counter++;
		if(n<0) {
			perror("Problem sendto\n");
			return 1;
		}
		if (ferror(fp)){
			printf("Error reading the file at server program\n");
			break;
		}
	} 	
	fclose(fp2);
	
	close(comm_fd);
	close(listen_fd);
	return 0;
}
