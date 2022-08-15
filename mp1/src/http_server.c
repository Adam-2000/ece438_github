/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXBUFSIZE 512
#define MAX_PATH2FILE 100
#define MAX_RESPONSELENGTH 100
#define MAXDATASIZE 4096
void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* concat(const char *s1, const char *s2)
{
    char *result = (char *)malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int main(int argc, char *argv[])
{
	if (argc > 2) {
	    fprintf(stderr,"File Name not specified\n");
	    exit(1);
	}

	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	//unsigned long len;
	

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	if(argc == 2){
		if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}
	} else {
		if ((rv = getaddrinfo(NULL, "80", &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}
	}
	

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			unsigned long len_receive;
			unsigned long len_path2file;
			char buf[MAXBUFSIZE];
			char path2file[MAX_PATH2FILE];
			char *pos, *pos1;
			FILE *fptr;
			char* result;
			if((len_receive =recv(new_fd, buf, MAXBUFSIZE - 1, 0)) == -1){
				perror("recv");
	    		exit(1);
			}
			char response[MAX_RESPONSELENGTH];
			strcpy(response, "HTTP/1.1 ");
			if(strncmp(buf, "GET ", 4) != 0){
				printf("Bad Request \n");
				strcat(response, "400 Bad Request\r\n\r\n");
				if (send(new_fd,response, strlen(response), 0) == -1)
					perror("send");
				close(new_fd);
				exit(0);
			}
			pos = strchr(buf, '/');
			pos1 = strchr(pos, ' ');
			len_path2file = (unsigned long)(pos1 - (++pos));
			strncpy(path2file, pos, len_path2file);
			path2file[len_path2file] = '\0';
			fptr = fopen(path2file, "r");
			if(fptr == NULL){
				printf("Cannot open file \n");
				strcat(response, "404 Not Found\r\n\r\n");
				result = concat(response, "Whoops, file not found!");
				if (send(new_fd,result, strlen(result), 0) == -1)
					perror("send");
				close(new_fd);
				fclose(fptr);
				free(result);
				exit(0);
			}
			fseek(fptr, 0, SEEK_END);
			unsigned long len_file = (unsigned long)ftell(fptr);	
			fseek(fptr, 0, SEEK_SET);
			char *string = (char *)malloc(len_file+1);
			fread(string,len_file,1,fptr);
			string[len_file] = '\0';
			char senddata[MAXDATASIZE+1];
			int cnt = 0;
			strcat(response, "200 OK\r\n\r\n");
			strncpy(senddata, string + cnt, MAXDATASIZE);
			senddata[MAXDATASIZE] = '\0';
			cnt += MAXDATASIZE;
			result = concat(response, senddata);
			// result = concat(response, string);
			if (send(new_fd,result, strlen(result), 0) == -1)
				perror("send");
			while(cnt < len_file + 1){
				strncpy(senddata, string + cnt, MAXDATASIZE);
				senddata[MAXDATASIZE] = '\0';
				cnt += MAXDATASIZE;
				if (send(new_fd,senddata, strlen(senddata), 0) == -1)
						perror("send");
				
			}
		//END:;
			//printf("response:\n %s\n", result);	
			
			close(new_fd);
			fclose(fptr);
			free(result);
			free(string);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

