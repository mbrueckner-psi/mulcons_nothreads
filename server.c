#include <stdio.h> 
#include <string.h> //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h> //close 
#include <arpa/inet.h> //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
	
#define TRUE 1 
#define FALSE 0 
#define PORT 8888 

int fpga_read_temp()
{
	return 50;
}

// Acquisition if faked by decrementing a counter whenever acq status is checked.

int acq_countdown = 0;

void fpga_start_acquire()
{
	printf("Start acq\n");
	acq_countdown = 200;
	return;
}

int fpga_check_acquire_state()
{
	printf("Check acq %d\n", acq_countdown);
	acq_countdown--;
	if (acq_countdown == 0)
		return 1;
	else
		return 0;
}

void fpga_stop_acquire()
{
	acq_countdown = 0;
}


int main(int argc , char *argv[]) 
{ 
	int opt = TRUE;
	int master_socket, addrlen, new_socket, client_socket[30], max_clients = 30, activity, i, valread, sd;
	// Store the socket of the client which started the acquisition
	int acq_socket_no = -1;
	int ret;
	int max_sd;
	// timeout of 0 for select()
	struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
	struct sockaddr_in address;
	int temp;
		
	char buffer[1025]; //data buffer of 1K 
		
	//set of socket descriptors 
	fd_set readfds; 
		
	////a message 
	//char *message = "ECHO Daemon v1.0 \r\n"; 


	//initialise all client_socket[] to 0 so not checked 
	for (i = 0; i < max_clients; i++) 
	{ 
		client_socket[i] = 0; 
	} 
		
	//create a master socket 
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	//set master socket to allow multiple connections , 
	//this is just a good habit, it will work without this 
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
		sizeof(opt)) < 0 ) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	
	//type of socket created 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( PORT ); 
		
	//bind the socket to localhost port 8888 
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	printf("Listener on port %d \n", PORT); 
		
	//try to specify maximum of 3 pending connections for the master socket 
	if (listen(master_socket, 3) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	} 
		
	//accept the incoming connection 
	addrlen = sizeof(address); 
	puts("Waiting for connections ..."); 
		
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
	
		// Check for any activity on any sockets. Timeout is set to 0, so select won't block
		// For a blocking select() set timeout parameter to NULL
		activity = select( max_sd + 1 , &readfds , NULL , NULL , &timeout); 
	
		if ((activity < 0) && (errno!=EINTR)) 
		{ 
			printf("select error"); 
		} 
			
		//If something happened on the master socket , 
		//then its an incoming connection 
		if (FD_ISSET(master_socket, &readfds)) 
		{ 
			if ((new_socket = accept(master_socket, 
					(struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
			{ 
				perror("accept"); 
				exit(EXIT_FAILURE); 
			} 
			
			//inform user of socket number - used in send and receive commands 
			printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs 
				(address.sin_port)); 
		
/*			//send new connection greeting message 
			if( send(new_socket, message, strlen(message), 0) != strlen(message) ) 
			{ 
				perror("send"); 
			} 
				
			puts("Welcome message sent successfully"); */
				
			//add new socket to array of sockets 
			for (i = 0; i < max_clients; i++) 
			{ 
				//if position is empty 
				if( client_socket[i] == 0 ) 
				{ 
					client_socket[i] = new_socket; 
					printf("Adding to list of sockets as %d\n" , i); 
						
					break; 
				} 
			} 
		} 
			
		//else its some IO operation on some other socket 
		for (i = 0; i < max_clients; i++) 
		{ 
			sd = client_socket[i]; 
				
			if (FD_ISSET( sd , &readfds)) 
			{ 
				//Check if it was for closing , and also read the 
				//incoming message 
				if ((valread = read( sd , buffer, 1024)) == 0) 
				{ 
					//Somebody disconnected , get his details and print 
					getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen); 
					printf("Host disconnected , ip %s , port %d \n" , 
						inet_ntoa(address.sin_addr) , ntohs(address.sin_port)); 
					
					// If acquisition client has closed the connection, stop acquisition
					if (acq_socket_no == i)
					{
						acq_socket_no = -1;
						fpga_stop_acquire();
					}
					//Close the socket and mark as 0 in list for reuse 
					close( sd ); 
					client_socket[i] = 0; 
				} 
					
				// Handle message input 
				else
				{ 
					// Command parsing
					switch(buffer[0])
					{
						case 'a' :
							// Start acquire
							// Check if acquire has been started by another client already
							if (acq_socket_no == -1)
							{
								fpga_start_acquire();
								acq_socket_no = i;
							}
							else
							{
								// If acquisition has been started by another client, just close connection
								// a message like "Acquisition already started" would be nice
								close(sd);
								client_socket[i] = 0;
							}

							break;
						case 't' :
							// Send temperature and close connection afterwards
							temp = fpga_read_temp();
							sprintf(buffer, "FPGA Temperature: %d °C\n", temp);
							send(sd, buffer, strlen(buffer), 0);
							close(sd);
							client_socket[i] = 0;
							break;
						case 's' :
							// Stop acquire
							// First check if an acquisition is running
							if (acq_socket_no > -1)
							{
								// Stop it and close connection to the acq client (stop blocking)
								fpga_stop_acquire();
								close(client_socket[acq_socket_no]);
								client_socket[acq_socket_no] = 0;
								acq_socket_no = -1;
							}
							// Close connection to the 's' sending client
							close(sd);
							client_socket[i] = 0;

							break;
						case 'x' :
							// Check status and close connection
							if (acq_socket_no > -1)
								sprintf(buffer, "Acquisition running\n");
							else
								sprintf(buffer, "No acquisition running\n");
							send(sd, buffer, strlen(buffer), 0);
							close(sd);
							client_socket[i] = 0;

							break;
						default:
							// Invalid command
							// A message like "invalid command" would be nice
							close(sd);
							client_socket[i] = 0;
							break;
					}
					//set the string terminating NULL byte on the end 
					//of the data read 
					//buffer[valread] = '\0'; 
					//send(sd , buffer , strlen(buffer) , 0 ); 
				} 
			} 
		} 

		// Check for acquisition status on every loop
		if (acq_socket_no > -1)
		{
			ret = fpga_check_acquire_state();
			if (ret > 0)
			{
				// If acquisition done send return value to client and close connection
				send(client_socket[acq_socket_no], &ret, sizeof(ret), 0);
				close(client_socket[acq_socket_no]);
				client_socket[acq_socket_no] = 0;
				acq_socket_no = -1;
			}
		}

		// should be done nicer
		// Just wait a little bit
		usleep(100*1000);

	} 
		
	return 0; 
} 

