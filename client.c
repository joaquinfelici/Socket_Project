/*
 * @author Joaquin Rodriguez Felici <joaquinfelici@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#define BOLDRED "\033[1m\033[31m"  
#define BOLDGREEN  "\033[1m\033[32m"  
#define RESET "\x1B[0m"

#define SIZE 1024

/*
 * Socket information
 */
int c_client_socket;
struct sockaddr_in server_struct;
int port = 0;

/*
 * Information for non-connection oriented socket
 */
int nc_client_socket;
struct sockaddr_in nc_server_struct;

/*
 * Buffers to store information
 */
char in[SIZE];
char* word;
char username[SIZE];
char ip[SIZE];
char password[SIZE];

/*
 * Functions to establish connection
 */
void connection_data();
void create_socket();
int create_nc_socket();
void connection();
void recieve_file();

/*
 * Auxiliary functions
 */
int random_number(int, int);

//-----------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
	connection_data();
	create_socket();
	connection();
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief This function parses the inicial command (connect user@host:port)
 */
void connection_data()
{
	printf(" > Client main started\n");

	printf("Insert command: ");
    fgets(in, SIZE, stdin);
	in[strlen(in)-1] = '\0';

	word = strtok(in, " ");
	if(!strcmp("connect", word))
	{
		word = strtok(NULL , "@");
        strcpy(username, word);
		word = strtok(NULL ,  ":");
        strcpy(ip, word);
		word = strtok(NULL, "\0");
		port = atoi(word);
	}
    else
	{
		printf(" > Unknown command, correct syntax: \n");
		printf(" > connect username@ip:port \n");
	}
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Creates the connection-oriented socket, and binds it to a port
 * @detail It also establishes the first communication with the server by sending the username and password
 * and analyzing it's response to tell if the login process was successful
 */
void create_socket()
{
	c_client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(c_client_socket < 0)
	{
		perror(" > Error while opening socket \n > Exiting \n");
		exit(1);
	}

	memset((char *) &server_struct, '0', sizeof(server_struct));
	server_struct.sin_family = AF_INET;

	if(!strcmp(ip, "localhost"))
	{
		struct hostent* local = gethostbyname(ip);
		bcopy((char*) local->h_addr, (char*) &server_struct.sin_addr.s_addr, local->h_length);
	}
	else
		server_struct.sin_addr.s_addr = inet_addr(ip);

	server_struct.sin_port = htons(port);

	if (connect(c_client_socket, (struct sockaddr*) &server_struct, sizeof(server_struct)) < 0 ) 
	{
		perror(" > Error while connecting \n > Exiting \n");
		exit(1);
	}

	printf(" > Connection successfully established \n");

	/*
	* Try to log-in
	*/
	printf("Input password: ");
    fgets(password, SIZE, stdin);

    char request[SIZE] = "";
    char reply[SIZE];

    password[strlen(password)-1] = '\0';

    /*
	* Login info is sent to server as username@password
	*/
    strcat(request,username);
    strcat(request,"@");
    strcat(request,password);

	int n = write(c_client_socket, request, strlen(request));
	
	n = read(c_client_socket, reply, SIZE);

	n = n;

	if(!strcmp("accepted", reply))
	{
		printf(" > Successfully logged in to server \n");
	}
	else
	{
		printf(" > Username or password is incorrect \n");
		exit(1);
	}
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief This is the main function, an infitnite while loop that prints the console emulator, recieves commands
 * sends it to the server, and hear's the reply. The while breaks in case of "disconnect" command
 */
void connection()
{
	char buffer[SIZE];
	static int flag = 1;

	while(1) 
	{
		if(flag)
		{
			printf(" > Available commands are: \n      > listar\n      > descargar num_estacion\n      > diario_precipitacion num_estacion\n      > mensual_precipitacion num_estacion\n      > promedio variable\n      > disconnect\n");
			flag = 0;
		}

		printf(BOLDGREEN "%s" "@" "%s: " RESET BOLDRED "$ " RESET, username, ip);	
		memset(buffer, '\0', SIZE);
		fgets(buffer, SIZE - 1, stdin);

		int n = write(c_client_socket, buffer, strlen(buffer));
		if (n < 0) 
		{
			perror(" > Error while writing to socket \n > Exiting");
			exit(1);
		}

		if(strstr(buffer, "disconnect"))
		{
			exit(1);
		}
		else
		{
			memset(buffer, '\0', SIZE);
			n = read(c_client_socket, buffer, SIZE);
			if ( n < 0 )
			{
				perror(" > Error while reading socket \n > Exiting \n");
				exit(1);
			}
				printf(" > Recieved data: %s\n", buffer);

			/*
			 * If the server recognized the command, then it's asking for our port
			 */
			if(strcmp("unknown", buffer))
			{
				/*
				 * We create the socket and get the port
				 */
				int nc_port = create_nc_socket();
				char nc_port_number[64];
				sprintf(nc_port_number,"%d",nc_port);
				int n = write(c_client_socket, nc_port_number, strlen(nc_port_number));
				n = n;
				
				recieve_file();
			}			
		}
	}
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Function to recieve a CSV file
 * @detail Roughly, it writes a new line to a file everytime it recieves a line different than "end"
 */
void recieve_file()
{
	char buffer[SIZE];
	int address_size = sizeof(struct sockaddr);
	int l;
	int flag = 0;

	FILE *file;
	char file_to_write[SIZE];
	sprintf(file_to_write, "recieved_file_%d.CSV", getpid());
	file = fopen(file_to_write, "w");	

	printf("--------------------------------- begin -----------------------------------\n");

	while(flag == 0) 
	{
		memset(buffer, 0, SIZE);
		l = recvfrom(nc_client_socket, (void*) buffer, SIZE, 0, (struct sockaddr*) &nc_server_struct, (socklen_t*) &address_size);
		if (l < 0) 
		{
			perror(" > Error while reading socket \n > Exiting \n");
			exit(1);
		}

		if(!strcmp("end", buffer))
			flag = 1; 
		else
		{
			printf("%s",buffer);
			fprintf(file,"%s",buffer);
			fflush(file);
		}	

		l = sendto(nc_client_socket, (void*) "ack", 4, 0, (struct sockaddr*) &nc_server_struct, sizeof(nc_server_struct));
		if (l < 0)
		{
			perror(" > Error while writing to socket \n > Exiting \n");
			exit(1);
		}
	}

	printf("---------------------------------- end ------------------------------------\n");
	printf(" > Successfully recieved file %s\n", file_to_write);
	fclose(file);
	close(nc_client_socket);
}		

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief produces a random value between the specified limits
 * @param min_num is the lower limit
 * @param max_num is the highest limit
 */
int random_number(int min_num, int max_num)
{
	int result = 0, low_num = 0, hi_num = 0;

	if(min_num < max_num)
	{
        low_num = min_num;
        hi_num = max_num + 1; 
	}
	else
	{
        low_num = max_num + 1;
        hi_num = min_num;
	}

	srand(time(NULL));
	result = (rand() % (hi_num - low_num)) + low_num;
	return result;
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Initilializes all necessary variables to establish non-connection oriented socket 
 */
int create_nc_socket()
{
	int port_number;
	port_number = random_number(6050,30000);

	nc_client_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if(nc_client_socket < 0) 
	{
		perror(" > Error while reading socket \n > Exiting \n");
	}

	memset(&nc_server_struct, 0, sizeof(nc_server_struct));
	nc_server_struct.sin_port = htons(port_number);		
	nc_server_struct.sin_family = AF_INET;
	nc_server_struct.sin_addr.s_addr = INADDR_ANY;
	memset(&(nc_server_struct.sin_zero), '\0', 8);

	while(bind(nc_client_socket, (struct sockaddr*) &nc_server_struct, sizeof(nc_server_struct)) < 0)
	{
		nc_server_struct.sin_port = htons(port_number);
	}

	printf(" > Ready to recieve file on port %d\n", ntohs(nc_server_struct.sin_port));
	return ntohs(nc_server_struct.sin_port);
}