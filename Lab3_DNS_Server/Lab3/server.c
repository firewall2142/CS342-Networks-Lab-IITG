#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <stdbool.h>


int search_database(char* request_msg, char* queried_object, int type_of_msg) {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int db_status = 0;
	
	printf("[REQUESTED FOR]: %s\n", request_msg);
	fp = fopen("./database.txt", "r");
		
	if (fp == NULL) {
		printf("Unable to open File\n");
		return -1;
	}

	
	// Reading the database.txt file
	while ((read = getline(&line, &len, fp)) != -1) {
		char *exists = strstr(line, request_msg);
		
		if(exists) {
			int idx = line - exists;
			int flg = 0;
				
			if(idx == 0 && type_of_msg == 1) {
				memcpy(queried_object, &line[strlen(request_msg)], strlen(line) - strlen(request_msg) - 1);
				flg = 1;
				}
			else if(idx != 0 && type_of_msg == 2){
				memcpy(queried_object, &line[0], strlen(line) - strlen(request_msg) - 1);
				flg = 1;
			}
				
			if(flg) {
				queried_object[strlen(line) - strlen(request_msg) - 1] = '\0';
				db_status = 1;
				break;
			}
		}
		else {
			strcpy(queried_object, "Entry Not Found");
		}
	}
	fclose(fp);
	
	return db_status;
}


int main(int argc, char const *argv[]) 
{ 
	int socket_fd, connection_fd; 
	struct sockaddr_in serverAddress, clientAddress; 
	char *ERROR = "-#Database corrputed";
	
	// Validating User Parameters
	if(argc != 2) {
		printf("[USAGE]: <executable code> <Server Port number>\n");
		return 0;
	}
	
	int PORT_NO = atoi(argv[1]);
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(socket_fd < 0) { 
		printf("[ERROR]: Unable to create socket\n");
		exit(EXIT_FAILURE); 
	}
	else {
		printf("[SUCCESS]: Socket created\n");
	}
	
	// Configuring socket parameters
	serverAddress.sin_family = AF_INET; 
	serverAddress.sin_addr.s_addr = INADDR_ANY; 
	serverAddress.sin_port = htons( PORT_NO ); 
	
	
	// Binding the socket to the specified port
	int bind_res = bind(socket_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	if(bind_res < 0) { 
		printf("[ERROR]: Failed to bind to the socket\n"); 
		exit(EXIT_FAILURE); 
	} 
	else {
		printf("[SUCCESS]: Successfully binded\n");
	}
	
	
	// Listening for the requests from the DNS Proxy
	if(listen(socket_fd, 5) < 0) { 
		printf("[ERROR]: Unable to Listen\n");
		exit(EXIT_FAILURE); 
	} 
	else {
		printf("[SUCCESS]: Listening\n");
	}
	
	
	// Infinite loop to query for multiple requests from the clients
	while(1) {
		int clientAddress_len = sizeof clientAddress ;
		connection_fd =  accept(socket_fd, (struct sockaddr *)&clientAddress, &clientAddress_len);
	
		if(connection_fd < 0) { 
			printf("[ERROR]: Refused to connect\n"); 
			exit(EXIT_FAILURE); 
		} 
		else {
			printf("[SUCCESS]: Connection Established\n");
		}
		
		
		char *queried_object = (char *) malloc(1024);
		char buffer[1024] = {0}; 
		memset(buffer, 0, strlen(buffer));
		
		int recv_status = recv(connection_fd, buffer, 1024, 0); 	
		int type_of_msg = buffer[0] - '0';
		char request_msg[strlen(buffer) - 2];
		//printf("[DEBUGGING]: %s\t%d\t%d\n", buffer, strlen(buffer), recv_status);
		
		if(recv_status != 0){
			memcpy(request_msg, &buffer[2], strlen(buffer) - 2);
			request_msg[strlen(buffer) - 2] = '\0';
		}
		
		printf("[PROGRESS]: Message type received = %d\n", type_of_msg);
		if(type_of_msg == 1 ) {
			printf("Domain Name = %s\n", request_msg);
			printf("[SEARCHING]...\n\n");
		}
		else if (type_of_msg == 2) {
			printf("I/P Address = %s\n", request_msg);
			printf("[SEARCHING]...\n\n");
		}
		else {
			printf("[CLOSE]: Client is down\n\n");
			break;
		}
		
		
		// Searching in Database
		int server_status = search_database(request_msg, queried_object, type_of_msg);
		
		if(server_status == -1) {
			send(connection_fd, ERROR, strlen(ERROR), 0);
			break;
		}
		
		
		// Configuring the Reply from the DNS Server
		char *reply = (char *) malloc(1024);
		
		reply[0] = '4' - server_status;
		reply[1] = '#';
		strcpy(reply + 2, queried_object);
		printf("[RESULT]: %s\n", reply);
		printf("[RESULT]: Message type sent = %d\n", reply[0] - '0');
		
		
		// Replying to the DNS Proxy
		send(connection_fd, reply, strlen(reply), 0); 
		
		close(connection_fd);
	}
	
	printf("[COMPLETED]: Server Closed\n"); 
	close(socket_fd);
	
	return 0; 
} 
