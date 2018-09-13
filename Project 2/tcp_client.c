#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include<string.h>
#include <unistd.h> //for write and read declaration (otherwise implicit declaration)
#include <arpa/inet.h> //for inet_pton
#include <time.h>
#include <stdlib.h>

#define SEND_LINE_SIZE 1024
#define RECV_LINE_SIZE 1024

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
	return 10;
}
 
int main(int argc,char* argv[]) {
	if (argc<4){
		fprintf(stderr, "You must provide an IP address, port number, and filename\n");
		return 1;
	}
	
	int sockfd;
	char sendline[SEND_LINE_SIZE];
	char recvline[RECV_LINE_SIZE];
	int  bytesReceived = 0;
	struct sockaddr_in servaddr;
	int file_size;
	
	clock_t start = clock(), diff;
 
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof servaddr);
 
	servaddr.sin_family=AF_INET;
	//servaddr.sin_port=htons(8080);
	servaddr.sin_port=htons(atoi(argv[2]));
 
	//inet_pton(AF_INET,"127.0.0.1",&(servaddr.sin_addr)); //inet_pton converts IPv4 and IPv6 addresses from text to binary form
	inet_pton(AF_INET,argv[1],&(servaddr.sin_addr));
	/*configure socket to reuse ports*/
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

	
	FILE *fp = fopen(argv[3],"r");
	if(fp==NULL){
		fprintf(stderr,"Zip Not Found\n");
		return 1;
	}

	//determine the requested file size
	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	
	//send the number of bytes in the file size, and the file size
	int filebytes = numPlaces(file_size);
	sprintf(sendline, "%d", filebytes);
	sprintf(recvline, "%d", file_size);
	strncat(sendline, recvline, filebytes);
	int n = send(sockfd,sendline, strlen(sendline), 0);
	if(n<0) {
		perror("Problem sendto\n");
		return 1;
	}
	if (ferror(fp)){
		printf("Error reading the file at server program\n");
		return 1;
	}
	int counter = 1;

	while (!feof(fp)){
		/*Read file in chunks of 100 bytes. If read was success, send data. */
		int nread = fread(sendline,1,SEND_LINE_SIZE,fp);
		printf("Bytes read %d \n", nread);
		/*if it's the end of the file, put a 1 flag at the end*/
		if (feof(fp)){
			printf("End of file\n");
			int n = send(sockfd,sendline, nread, 0);
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
		int n = send(sockfd,sendline, SEND_LINE_SIZE, 0);
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
	
	fclose(fp);
	printf("Zip sent!\n");
	
	/* Create file where data will be stored */
	FILE *fp2;
	fp2 = fopen("unzipped_file.txt", "w"); 
	if(NULL == fp2){
        	printf("Error opening the file");
        	return 1;
	}
    	counter = 1;
	int filesize;
	int filebyte;
	int only8;
    
    do {
		/* Receive data in chunks of 1024  bytes */
		bytesReceived =recv(sockfd,recvline,RECV_LINE_SIZE,0);
		if(counter ==1 && bytesReceived <1024){
			printf("Number of bytes received: %d\n", bytesReceived);
			strncpy(sendline, recvline, 1);
			strncat(sendline, "/0", 1);
			sscanf(sendline, "%d", &filebyte);
			printf("file bytes is %d\n", filebyte);
			strncpy(sendline, &recvline[1], filebyte);
			sscanf(sendline, "%d", &filesize);
			printf("file size is %d\n", filesize);
			only8=0;
		}else if (counter ==1 && bytesReceived ==1024){
			printf("Number of bytes received: %d\n", bytesReceived);
			strncpy(sendline, recvline, 1);
			strncat(sendline, "/0", 1);
			sscanf(sendline, "%d", &filebyte);
			printf("file bytes is %d\n", filebyte);
			strncpy(sendline, &recvline[1], filebyte);
			sscanf(sendline, "%d", &filesize);
			printf("file size is %d\n", filesize);

			filesize = filesize - bytesReceived-(filebyte+1);
			if(fwrite(&recvline[filebyte+1],1,bytesReceived-(filebyte+1),fp)<0){
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

			if(fwrite(recvline,1,bytesReceived,fp)<0){
				printf("Error writing the file\n");
				return 1;
			}
		}
		memset(recvline, '0', sizeof(recvline));
		counter++;
 
    }while(filesize>0);
	printf("End of file\n");
	if (only8 ==0){
		printf("There was only 8\n");	
	}else{
		printf("There was 1024\n");
	}
	fclose(fp2);
	diff = clock() - start;

	int msec = diff * 1000 / CLOCKS_PER_SEC;
	printf("Time taken: %d milliseconds\n", msec%1000);
	close(sockfd);
	return 0;
}
