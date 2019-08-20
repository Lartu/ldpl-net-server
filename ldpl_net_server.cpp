// Server side C/C++ program to demonstrate Socket programming
// Based on https://www.geeksforgeeks.org/socket-programming-cc/
// and https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/

//Example code: A simple server side code, which echos back the received message. 
//Handle multiple socket connections with select and fd_set on Linux 
#include <stdio.h> 
#include <string.h> //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h> //close 
#ifndef _WIN32
#include <arpa/inet.h> //close
#include <sys/socket.h>
#include <netinet/in.h>
#pragma comment (lib, "Ws2_32.lib")
#else
#include <windows.h>
#include <Winsock2.h>
#include <ws2tcpip.h>
#endif
#include <sys/types.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <string>
#include <iostream>

#define TRUE 1 
#define FALSE 0 
#define PORT 8888
#define MAXCLIENTS 255

using namespace std;

int client_socket[MAXCLIENTS];

string LDPL_NET_MSG;
string LDPL_NET_IP;
double LDPL_NET_PORT = 8080;
double LDPL_NET_SN;

void LDPL_NET_CLIENT_CONNECTED();
void LDPL_NET_CLIENT_DISCONNECTED();
void LDPL_NET_ONMESSAGE();

void client_connected(unsigned int socket_number, string ip, unsigned int port){
	LDPL_NET_IP = ip;
	LDPL_NET_PORT = port;
	LDPL_NET_SN = socket_number;
    LDPL_NET_CLIENT_CONNECTED();
}

void client_disconnected(unsigned int socket_number, string ip, unsigned int port){
    LDPL_NET_IP = ip;
	LDPL_NET_PORT = port;
	LDPL_NET_SN = socket_number;
    LDPL_NET_CLIENT_DISCONNECTED();
}

void send_message(unsigned int socket_number, string message){
    if(send(socket_number, message.c_str(), strlen(message.c_str()), 0) != strlen(message.c_str()) ) 
    { 
        perror("send"); 
    }
}

void LDPL_NET_SENDMESSAGE(){
	send_message(LDPL_NET_SN, LDPL_NET_MSG);
}

void LDPL_DISCONNECT_CLIENT(){
	#ifdef _WIN32
	closesocket(LDPL_NET_SN);
	#else
	close(LDPL_NET_SN);
	#endif
	for (int i = 0; i < MAXCLIENTS; i++) 
	{
		if (client_socket[i] == LDPL_NET_SN){
			client_socket[i] = 0;
		}
	}
}

void client_onmessage(unsigned int socket_number, string message){
	LDPL_NET_SN = socket_number;
	LDPL_NET_MSG = message;
    LDPL_NET_ONMESSAGE();
}
	
void LDPL_NET_STARTSERVER() 
{ 
	int opt = TRUE; 
	int master_socket , addrlen , new_socket , 
		max_clients = MAXCLIENTS , activity, i , valread , sd; 
	int max_sd; 
	struct sockaddr_in address; 
		
	char buffer[1025]; //data buffer of 1K 
		
	//set of socket descriptors 
    //Set of file descriptors to monitor socket activity
	fd_set readfds;
	
	//initialise all client_socket[] to 0 so not checked 
	for (i = 0; i < max_clients; i++) 
	{ 
		client_socket[i] = 0; 
	} 

	#ifdef _WIN32
	WSADATA wsaData;
	int iResult;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        exit(EXIT_FAILURE); 
    }
	ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
	iResult = getaddrinfo(NULL, to_string(int(LDPL_NET_PORT)).c_str(), &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        exit(EXIT_FAILURE); 
    }
	#endif
		
	//create a master socket 
	if((master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
	{ 
		perror("socket failed");
		#ifdef _WIN32
        WSACleanup();
		#endif
		exit(EXIT_FAILURE); 
	} 

	#ifndef _WIN32
	//set master socket to allow multiple connections, 
	//this is just a good habit, it will work without this
	if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	}
	#endif
	
	//type of socket created 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( (uint16_t) LDPL_NET_PORT ); 
		
	//bind the socket to localhost port LDPL_NET_PORT 
	iResult = bind(master_socket, (struct sockaddr *) &address, sizeof(address));
	if (iResult < 0)
	{ 
		perror("bind failed");
		#ifdef _WIN32
        WSACleanup();
		#endif
		exit(EXIT_FAILURE); 
	} 
	//printf("Listener on port %d \n", PORT); 
		
	//try to specify maximum of 16 pending connections for the master socket 
	if (listen(master_socket, 16) < 0) 
	{ 
		perror("listen"); 
		#ifdef _WIN32
        WSACleanup();
		#endif
		exit(EXIT_FAILURE); 
	} 
		
	//accept the incoming connection 
	addrlen = sizeof(address); 
	//puts("Waiting for connections ..."); 
		
	while(TRUE) 
	{ 
		//clear the socket set 
		FD_ZERO(&readfds); 
	
		//add master socket to set 
		FD_SET(master_socket, &readfds); 
		max_sd = master_socket; 
			
		//add child sockets to set 
		for ( i = 0 ; i < max_clients ; i++) 
		{ 
			//socket descriptor 
			sd = client_socket[i]; 
				
			//if valid socket descriptor then add to read list 
			if(sd > 0) 
				FD_SET( sd , &readfds); 
				
			//highest file descriptor number, need it for the select function 
			if(sd > max_sd) 
				max_sd = sd; 
		} 
	
		//wait for an activity on one of the sockets , timeout is NULL , 
		//so wait indefinitely 
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL); 
	
		if ((activity < 0) && (errno!=EINTR)) 
		{ 
			perror("select error"); 
		} 
			
		//If something happened on the master socket , 
		//then its an incoming connection 
		if (FD_ISSET(master_socket, &readfds)) 
		{ 
			#ifdef _WIN32
			new_socket = accept(master_socket, NULL, NULL);
			#else
			new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
			#endif
			if (new_socket < 0) 
			{ 
				perror("accept"); 
				#ifdef _WIN32
				WSACleanup();
				#endif
				exit(EXIT_FAILURE); 
			} 
			
			//inform user of socket number - used in send and receive commands
            client_connected(new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port)); 
				
			//add new socket to array of sockets 
			for (i = 0; i < max_clients; i++) 
			{ 
				//if position is empty 
				if( client_socket[i] == 0 ) 
				{ 
					client_socket[i] = new_socket; 
					//printf("Adding to list of sockets as %d\n" , i);
					break; 
				} 
			} 
		} 
			
		//else its some IO operation on some other socket 
		for (i = 0; i < max_clients; i++) 
		{ 
			sd = client_socket[i]; 
				
			if (FD_ISSET(sd, &readfds)) 
			{ 
				//Check if it was for closing , and also read the 
				//incoming message 
				#ifdef _WIN32
				valread = recv(sd, buffer, 1024, 0);
				#else
				valread = read(sd, buffer, 1024);
				#endif
				if (valread == 0) 
				{ 
					//Somebody disconnected , get his details and print 
					getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen); 
                    client_disconnected(sd, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
						
					//Close the socket and mark as 0 in list for reuse 
					close(sd); 
					client_socket[i] = 0; 
				} 
					
				//Echo back the message that came in 
				else
				{ 
					//set the string terminating NULL byte on the end 
					//of the data read 
					buffer[valread] = '\0';
                    string message = buffer;
                    client_onmessage(sd, message);
				} 
			} 
		} 
	}
} 
