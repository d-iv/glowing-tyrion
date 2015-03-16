/*

	
	FTP server application. Creates socket and transfers files on request from client
	application stops when user enters control-z
	
	Beej's guide used to develop the program guide
	//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h> 
#include <signal.h>
#include <sys/stat.h> 
#include <fcntl.h>

//global variable to set server loop
int go = 1;


/******************************************************************************************
	set up and bind server socket
	Result: socket file descriptor after bind
*******************************************************************************************/
int setupServe(char* port){
	
	//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
	//ports below 1024 reserved max port number 65535
	int status;
	struct addrinfo hints;
	struct addrinfo *res;  // will point to the results
	int socket_fd;
	
	//set hints
	memset(&hints, 0, sizeof(struct addrinfo) ); // make sure the struct is empty
	hints.ai_family = AF_INET;     
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	//get address info
	// res will point to a linked list of 1 or more struct addrinfos
	if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}	
	
	//set socket
	socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol); 
	
	//bind socket
	if(bind(socket_fd, res->ai_addr, res->ai_addrlen) == -1){
		perror("BIND FAILED: ");
		return -1;
	}
	freeaddrinfo(res); // free the linked-list
	
	return socket_fd;
}

/******************************************************************************************
	Accept for server
	Result: socket file descriptor after accept
*******************************************************************************************/
int serveAccept(int socket_fd){
	//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	int socket_fd2;
	
	addr_size = sizeof(struct sockaddr_storage);
	socket_fd2 = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_size);
	
	return socket_fd2;
}

/******************************************************************************************
	send data with error checking for full amount of data sent
	returns bytes sent
*******************************************************************************************/
int mySend(int socket_fd, char* buf, int check, int flags){

	int bytes;
	bytes = send(socket_fd, buf, check, flags);
	if(bytes != check){
		printf("ERROR mySend: bytes sent less than expected\n");
		return -1;
	}
	return bytes;
}

/******************************************************************************************
	receive data with error checking for full amount of data received
	returns bytes sent
*******************************************************************************************/
int myRecv(int socket_fd, char* buf, int check, int flags){
	int bytes;
	memset(buf,'\0',sizeof(buf));
	bytes = recv(socket_fd, buf, check, flags);
	if(bytes != check){
		printf("ERROR myRecv: bytes rcv less than expected\n");
		return -1;
	}
	
	return bytes;
}

/******************************************************************************************
	Sends a file over specified socket
	First sends file size then full file in packets of 500 bytes
	Result: File transfered
*******************************************************************************************/
int sendFile(int socket_fd, char* fname){
	int size = 0;
	int bytes;
	int flags = 0;
	char buf[500];
	int check = sizeof(buf);
	int rfd;
	char *fileName = "blarf";
	int totalBytes = 0;
	
	struct stat finfo;
	
	
	//check file exists
	if(stat(fname, &finfo) == -1){
		printf("%s  :: Does not exist\n", fname);
		memset(buf,'\0',sizeof(buf));
		sprintf(buf, "-1");
		mySend(socket_fd, buf, check, flags);
		return -1;
	}
	//open file and get size used stack overflow posts noted below
	//http://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
	//http://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform
	rfd = open(fname, O_RDONLY);
	size = finfo.st_size;
	
	//clear buf and set to size of file
	memset(buf,'\0',sizeof(buf));	
	sprintf(buf, "%i", size);
	
	//send file size
	if(mySend(socket_fd, buf, check, flags) == -1){
		return -1;
	}
	
	//send file in packets of 500 bytes
	while(totalBytes <= size){
		bytes = 0;
		memset(buf,'\0',sizeof(buf));
		
		//read full packet
		if(totalBytes + check <= size){
			read(rfd, buf, check);
		}
		//read last bit of packet
		else if(size >= check){
			read(rfd, buf, size-totalBytes);
		}
		//if packet size was smaller than 500 only read file size
		else{
			read(rfd, buf, size);
		}
		bytes = mySend(socket_fd, buf, check, flags);
		totalBytes	+= bytes;		
	}
	
	//console note
	printf("_________Send file done_____\n");
	close(rfd);
	return 0;
}

/******************************************************************************************
	validate commands sent by client
	call other functions as needed.
*******************************************************************************************/
int validCommand(int socket_fd){
	
	int flags = 0;
	char buf[500];
	char vList[500];
	char vGet[500];
	int check = sizeof(buf);
	int out = 1; //stdout
	char* data = "30020";
	int socket_fd2, socket_fd3;
	int backlog = 5;
	
	sprintf(vList, "list\n");
	sprintf(vGet, "get");
	
	//get command
	if( myRecv(socket_fd, buf, check, flags) == -1){
		return -1;
	}
	
	//check if command is list	
	if(strcmp(vList, buf) == 0){
		socket_fd2 = setupServe(data);
		
		//send validated command back to client
		memset(buf,'\0',sizeof(buf));
		sprintf(buf, "vList");
		mySend(socket_fd, buf, check, flags);
		
		//start data socket
		if(listen(socket_fd2, backlog) == -1){
			perror("LISTEN FAILED:");
		}
		
		//create file directory list and send file
		//then delete
		socket_fd3 = serveAccept(socket_fd2);
		system("ls --hide blarf > blarf");
		printf("----Sending List----\n");
		sprintf(vList, "blarf");
		sendFile(socket_fd3, vList);
		system("rm -f blarf");
		
		//close sockets
		close(socket_fd2);
		close(socket_fd3);
	}
	//get command
	else if(strncmp(vGet, buf, 3) == 0){
		
		//return validated message
		mySend(socket_fd, buf, check, flags);
		
		//get file name
		memset(vGet,'\0',sizeof(vGet));
		int i=4;
		while(buf[i] != '\n' && buf[i] != '\0' && i < sizeof(buf)){
			vGet[i-4] = buf[i];
			i++;
		}
		
		//console message
		printf("----Sending File:  %s----\n", vGet);
		
		//send file
		sendFile(socket_fd, vGet);				
	}
	else{
		//send error message
		memset(buf,'\0',sizeof(buf));
		sprintf(buf, "ERROR: Invalid command");
		mySend(socket_fd, buf, check, flags);
	}
		
	return 0;
}

/******************************************************************************************
	signal handler for control-C
	Result: resets global variable go to -1
*******************************************************************************************/
void sigH_int(int signal){
	go = -1;
	return;
}//end void


//Main MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
int main(int argc, char *argv[])
{	
	int socket_fd;
	int socket_fd2;
	int backlog = 5;
	int blank;
	char test[1];
	char *port = "30021";
	//global variable go =1 declared at file start
	close(socket_fd2);
	close(socket_fd);
	
	//set up signal handler to end loop
	//if(signal(SIGTSTP, sigH_int) == SIG_ERR)
	if(signal(SIGINT, sigH_int) == SIG_ERR)
		exit(-1);
	
	//set and bind socket
	socket_fd = setupServe(port);
	if(socket_fd == -1){
		return -1;
	}
	
	//go is global variable changed by hitting ^z
	while (go == 1){	
		if(listen(socket_fd, backlog) == -1){
			perror("LISTEN FAILED:");
		}	
		socket_fd2 = serveAccept(socket_fd);
		while(recv(socket_fd2, test, 1, MSG_PEEK) > 0 ){
			validCommand(socket_fd2);  //validate commands			
		}
		close(socket_fd2);
		
	}//end while
	
	close(socket_fd2);
	close(socket_fd);		
	
	printf("\nEND ftServer!\n\n");	
	
    return 0;
}