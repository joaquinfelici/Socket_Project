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
void connection();

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
 * @brief 
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
 * @brief 
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

    //username[strlen(username)] = '\0';
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
 * @brief 
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
		else if (strstr(buffer, "listar"))
		{
			printf(" > Recieved data: \n");
			memset(buffer, '\0', SIZE);
			n = 1;
			while(n >= 0)
			{
				n = read(c_client_socket, buffer, SIZE);
				if(strstr(buffer, "end"))
					break;
				printf("%s \n", buffer);	
			}
			//printf(" > Recieved data: \n %s \n", buffer);
		}
	}		
}