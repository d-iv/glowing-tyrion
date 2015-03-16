/*
	
	
	FTP client. Contacts server and receives requested files
	interface provides commands for 
		list to t list directory files availble on server
		get <file> to retrieve file
		and quit to end program
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h>
 

/******************************************************************************************
	set up and bind client socket
	Result: socket file descriptor after bind
*******************************************************************************************/
int setupClient(char * port){
	//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
	///ports below 1024 reserved max port number 65535
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
	
	if(connect(socket_fd, res->ai_addr, res->ai_addrlen) == -1){
		perror("CONNECT FAILED!  ");
		return -1;
	}
	freeaddrinfo(res); // free the linked-list
	
	return socket_fd;
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
	rcv: recieves file from server
	
	Result: return -1 if not successful
		otherwise prints file to specified file location
*******************************************************************************************/
int rcv(int socket_fd, int wfd){
	int size = 0;
	int bytes;
	int totalBytes = 0;
	int flags = 0;
	char buf[500];
	char buf2[500];
	int check = sizeof(buf);
	
	int value;
	
	//get file size
	memset(buf,'\0',sizeof(buf));
	if(myRecv(socket_fd, buf, check, flags) ==-1){
		return -1;
	}
	
	//file size buffer send -1 if no file found
	size = atoi(buf);
	if(size < 0){
		printf("\nERROR: No Such File\n");
		return -1;
	}
	
	//receive rest of file
	while(totalBytes <= size){
		bytes = 0;
		memset(buf,'\0',sizeof(buf));
		
		bytes = myRecv(socket_fd, buf, check, flags);
		
		//print full packet to file
		if(totalBytes + check <= size){
			write(wfd, buf, check);
		}
		//print last partial packet
		else if(size >= check){
			write(wfd, buf, size-totalBytes);
		}
		//print small packet if file less than 500 bytes
		else{
			write(wfd, buf, size);
		}
		totalBytes	+= bytes;
	}
	return 0;
}

/******************************************************************************************
	validate commands
*******************************************************************************************/
int checkCommand(int socket_fd, char* command){
	
	int flags = 0;
	char buf[500];
	char vList[500] = "vList";
	int check = sizeof(buf);
	int out = 1; //stdout
	int err = 3;
	char* data = "30020";
	int socket_fd2;
	
	char vGet[500];
	sprintf(vGet, "get");
	
	//send command to server
	strcpy(buf, command);
	if( mySend(socket_fd, buf, check, flags) == -1){
		return -1;
	}
	
	//get validation or error message
	if( myRecv(socket_fd, buf, check, flags) == -1){
		return -1;
	}
	
	//check for list command 
	if(strncmp(vList, buf, 5) == 0){
		socket_fd2 = setupClient(data);
		
		//recieve list and out put to stout
		printf("\n----List of files----\n");
		rcv(socket_fd2, out);
		printf("\n");
		close(socket_fd2);
	}
	//look for get command
	else if(strncmp(vGet, buf, 3) == 0){
	
		//reference for creating files		
		//http://pubs.opengroup.org/onlinepubs/009696899/functions/open.html
		//get file name
		memset(vGet,'\0',sizeof(vGet));
		int i=4;
		while(buf[i] != '\n' && buf[i] != '\0' && i < sizeof(buf)){
			vGet[i-4] = buf[i];
			i++;
		}
		//create file or print duplicate message
		out = open(vGet, O_RDWR | O_CREAT | O_EXCL, 0666);
		if(out == -1){
			printf("\nERROR: Duplicate File\n");
			rcv(socket_fd, err);
			
			return 0;
		}
		//get data and print to file
		rcv(socket_fd, out);
		close(out);
		printf("\n\tTransfer Complete\n");
		
	}
	else{
		printf("\n\t%s\n", buf);
	}
	
	return 0;
}
//FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
int main(int argc, char *argv[])
{ 
	int socket_fd;
	int size = 500;
	char command[size];
	char *quit = "quit";
	int quitCount = 4;
	char* cmd = "30021";
	
	//close(socket_fd);
	close(socket_fd);
	
	//set up client socket
	socket_fd = setupClient(cmd);
	if(socket_fd == -1){
		printf("ERROR setupClient() \n\n");
		return -1;
	}
	
	//until quit receive commands from user
	while(strncmp(quit, command, quitCount) != 0){
		memset(command,'\0',size);
		printf("Enter a command (list, get <fileName>, quit) :: ");
		fgets(command, size, stdin);
		//validate commands on server
		if(strncmp(quit, command, quitCount) != 0){
			checkCommand(socket_fd, command);
		}
	}	
	
	close(socket_fd);	
	
	printf("\nEND ft_Client!\n\n");
    return 0;
}