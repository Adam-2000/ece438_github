/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 1024 // max number of bytes we can get at once 
#define MAXREQUESTMSGSIZE 100
#define MAXHOSTNAME 100
#define MAXPATH2FILE 100
#define PORTNUMBERLENGTH 4
#define HTTP_HOSTNAME_OFFSET 7
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	char hostname[MAXHOSTNAME];
	char portnumber[PORTNUMBERLENGTH + 1];
	char path2file[MAXPATH2FILE];
	char request[MAXREQUESTMSGSIZE];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	char* pos;
	unsigned int hostname_length; 
	//int flag_port_specified;
	unsigned int portnum_len;
	pos = strchr(argv[1]+HTTP_HOSTNAME_OFFSET, ':');
	if(pos == NULL){
		pos = strchr(argv[1]+HTTP_HOSTNAME_OFFSET, '/');
		strncpy(portnumber, "80", 3);
		hostname_length = (unsigned int)(pos - argv[1] - HTTP_HOSTNAME_OFFSET);
		strncpy(hostname, argv[1]+HTTP_HOSTNAME_OFFSET, hostname_length);
		hostname[hostname_length] = '\0';
	} else {
		hostname_length = (unsigned int)(pos - argv[1] - HTTP_HOSTNAME_OFFSET);
		strncpy(hostname, argv[1]+HTTP_HOSTNAME_OFFSET, hostname_length);
		hostname[hostname_length] = '\0';
		pos = strchr(argv[1]+HTTP_HOSTNAME_OFFSET + hostname_length, '/');
		portnum_len = (unsigned int)(pos - (argv[1]+HTTP_HOSTNAME_OFFSET + hostname_length + 1));
		strncpy(portnumber, argv[1]+HTTP_HOSTNAME_OFFSET + hostname_length + 1, portnum_len);
		portnumber[portnum_len] = '\0';
	}
	strncpy(path2file, pos, MAXPATH2FILE); 
	printf("hostmame:portnumber: %s:%s\n", hostname, portnumber);
	printf("path2file: %s\n", path2file);

	strcpy(request, "GET ");
	strcat(request, path2file);
	strcat(request, " HTTP/1.1\r\n\r\n");
	printf("sent_message: %s\n", request);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	
	if ((rv = getaddrinfo(hostname, portnumber, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
	if (send(sockfd,request, strlen(request), 0) == -1)
				perror("send");

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}
	if(numbytes > MAXDATASIZE-1) {
		numbytes = MAXDATASIZE - 1;
	}
	buf[numbytes] = '\0';

	FILE* fo = fopen("output", "wb");
	pos = strchr(buf, '\n');
	while(pos[1] != '\r'){
		pos = strchr(pos + 1, '\n');
	}
	pos += 3;
	fwrite(pos, sizeof(char), strlen(pos), fo);
	//fwrite(buf, sizeof(char), strlen(buf), fo);
	while (1){
		if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == 0 ){
			break;
		}
		if (numbytes  < 0) {
			perror("recv");
			fclose(fo);
			return 1;
		}
		fwrite(buf, sizeof(char), numbytes, fo);
	}
	fclose(fo);
	//printf("%s\n",buf);

	close(sockfd);

	return 0;
}

