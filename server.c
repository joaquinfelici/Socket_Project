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

#define SIZE 1024

/*
 * Predetermined username and password
 * TODO: Create list of usernames and list of passwords
 * where indexes are the same for both lists.
 */
#define username "joaquin\0"
#define password "felici\0"

/*
 * Socket information
 */
int c_server_socket, c_new_client_socket;
struct sockaddr_in server_struct, client_struct;
int port = 9000;
int client_length = sizeof(client_struct);

/*
 * functions to establish and keep connection
 */
void create_socket();
void listen_socket();
void login();
void process_command(const char*, char*);
void create_socket_n(); // Non oriented to conection

/*
 * functions to execute commands sent by client
 */
void list();
void download(int);
void daily();
void monthly();
void average(); 

/*
 * Auxiliary functions
 */
const char* getfield(char*, int);
void write_file(char*, const char*);

//----------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
	printf(" > Server main started\n");
	create_socket();

	while(1)
	{
		listen_socket();
	}

	return 0;
}

//-----------------------------------------------------------------------------------------------------------------

void listen_socket()
{
	c_new_client_socket = accept(c_server_socket, (struct sockaddr*) &client_struct, (socklen_t*) &client_length);

	if(c_new_client_socket < 0)
	{	
		perror(" > Error while accepting client \n > Exiting \n");
		exit(1);
	}

	int pid = fork();
	if(pid < 0)
	{
		perror(" > Error while forking process \n > Exiting \n");
		exit(1);
	}	

	if(pid != 0)
	{
		printf(" > Client with PID %d has connected\n", getpid());
		close(c_server_socket);

		login();

		char buffer_in[SIZE];
		char buffer_out[SIZE];

		while(1)
		{
			memset(buffer_in, 0, SIZE);
			int n = read(c_new_client_socket, buffer_in, SIZE - 1);
			if (n < 0)
			{
				perror(" > Error while reading socket \n > Exiting \n");
				exit(1);
			}

			printf(" > Data successfully recieved: %s", buffer_in);
			buffer_in[strlen(buffer_in) - 1] = '\0';
			
			/*
				Process command
			*/

			process_command((const char*) buffer_in, (char*) buffer_out);

			n = write(c_new_client_socket, buffer_out, 100);
			if ( n < 0 ) 
			{
				perror(" > Error while sendind data \n > Exiting \n");
				exit(1);
			}
			//printf(" > Data successfully sent: %s\n", buffer_out);
			printf(" > Data successfully sent \n");
		}
	}
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Recieves username and password, checks if the information belongs to a registered user
 */
void login()
{ 
	char buffer[SIZE];
	char user[SIZE];
	char pass[SIZE];

	int n = read(c_new_client_socket, buffer, SIZE-1);
	buffer[strlen(buffer)] = '\0';

	char *aux;

	aux = strtok(buffer, "@");
	strcpy(user, aux);
	aux = strtok(NULL , "\0");
    strcpy(pass, aux);

	n = n;
 
	if(!strcmp(username, user) && !strcmp(password, pass))
	{
		printf(" > User <%s> successfully logged in \n", username);
		n = write(c_new_client_socket, "accepted", SIZE);
	}
	else
	{
		printf(" > Authentication for user <%s> failed \n", user);
		n = write(c_new_client_socket, "refused", SIZE);
		process_command("disconnect", NULL); // Force the disconnect.
	}
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * Sets up the socket and binds it to a port
 */
void create_socket()
{
	// Create socket
	c_server_socket = socket(AF_INET, SOCK_STREAM, 0);

	// Set option to unbind port when we exit the program unexpectedly
	int iSetOption = 1;
	setsockopt(c_server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));


	if(c_server_socket < 0)
	{
		perror(" > Error while opening socket \n > Exiting \n");
		exit(1);
	}

	// Fill struct
l:	memset(&server_struct, 0, sizeof(struct sockaddr_in));
	server_struct.sin_family = AF_INET;
	server_struct.sin_addr.s_addr = INADDR_ANY;
	server_struct.sin_port = htons(port);

	// Bind
	int binding = bind(c_server_socket, (const struct sockaddr*) &server_struct, sizeof(server_struct));
	if(binding < 0)
	{
		perror(" > Error while binding socket \n > Exiting \n");
		port++;
		goto l;
		exit(1);
	}

	printf(" > Server socket successfully created in port %d \n", port);

	// Set to listen
	listen(c_server_socket, 10);
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Process the command to call the needed function
 * @param buffer_in the command that the client sent
 * @param buffer_out buffer to write to in order to reply
 */
void process_command(const char* buffer_in, char* buffer_out)
{
	// If the client wants to disconnect, then we just exit.
	if(!strcmp("disconnect", buffer_in))
	{
		write_file("holamijto lo amo", "puto.txt");
		printf(" > Client wih PID %d disconnected\n", getpid());	
		exit(1);
	}

	// Otherwise, let's get the different parts of the command

	// We copy the input buffer to avoid modifying the original
	char buffer[SIZE];
	strcpy(buffer, buffer_in);
	buffer[strlen(buffer_in)] = '\0';

	// Let's extract first word
	char *aux = strtok(buffer, " ");
	char command[SIZE];
	strcpy(command, aux);

	if(!strcmp("listar", buffer))
	{
		list();
		printf(" > Recieved command <listar> \n");	
		strcpy(buffer_out, "");
	}
	else  if(!strcmp("descargar", buffer))
	{
		// Let's extract station number
		aux = strtok(NULL, "\0");
		int station = -1;
		if(aux != NULL)
		station = atoi(aux);
		download(station);
		printf(" > Recieved command <descargar> for station %d \n", station);	
	}
	else  if(!strcmp("diario_precipitacion", buffer))
	{
		// Let's extract station number
		aux = strtok(NULL, "\0");
		int station = -1;
		if(aux != NULL)
		station = atoi(aux);

		printf(" > Recieved command <diario_precipitacion> for station %d \n", station);	
	}
	else  if(!strcmp("mensual_precipitacion", buffer))
	{
		// Let's extract station number
		aux = strtok(NULL, "\0");
		int station = -1;
		if(aux != NULL)
		station = atoi(aux);

		printf(" > Revieced command <mensual_precipitacion> for station %d \n", station);	
	}
	else  if(!strcmp("promedio", buffer))
	{
		// Let's extract the variable
		aux = strtok(NULL, "\0");
		char variable[SIZE];
		if(aux != NULL)
		strcpy(variable, aux);

		printf(" > Recieved command <promedio> for variable <%s> \n", variable);	
	}
	else
	{
		char file_to_write[SIZE];
		sprintf(file_to_write, "tmp_download_station_%d_%d.txt", station_number, getpid());
		remove((const char*) file_to_write);
		printf(" > Unknown command recieved \n");
		strcpy(buffer_out,"didn't get that dude");
	}
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Sends to the client a list of stations in file, as well as all the sensors that have valid data
 */
void list()
{
	FILE* stream = fopen("datos_meteorologicos_s.CSV", "r");

    char line[SIZE];
    char fields[SIZE];

    // Ignore first and second lines
    fgets(line, SIZE, stream);
    fgets(line, SIZE, stream);
    // Field names
    fgets(fields, 1024, stream);

    char last_station_found[SIZE]= "rubbish";

    int n = write(c_new_client_socket, " Stations:", 1024);

    while (fgets(line, SIZE, stream))
    {
        char* tmp = strdup(line);
        char* field = (char*) getfield(tmp,2);

        /*
         * If we still haven't found this station, we add this station to the file
         */
        if(strcmp(field, last_station_found))
        {
          char line_to_write[SIZE] = "";
          char aux[SIZE] = "";

          strcpy(last_station_found, field);
          char station_information[SIZE];
          strcat(station_information, field);

          sprintf(aux, "   - %s", last_station_found);
          n = write(c_new_client_socket, aux, SIZE);

          int i = 5;
          while(i < 20) // We analyze all fields for this station
          {
            tmp = strdup(line);
            field = (char*) getfield(tmp,i);

            if(strcmp(field, "--")) // We check if the field is completed or not
            {   
	              char* t = strdup(fields);
	              char* field_name = (char*) getfield(t,i);

				  sprintf(aux, "       %s", field_name);
	              n = write(c_new_client_socket, aux, SIZE);

	              sprintf(aux, "%s - ", field_name);
	              strcat(line_to_write, aux);              
            }
            i++;
          }
        n = n;
        }
        free(tmp);
    }
    n = write(c_new_client_socket, "end", SIZE);
}

//-----------------------------------------------------------------------------------------------------------------

void download(int station_number)
{
	FILE* stream = fopen("datos_meteorologicos_s.CSV", "r");
	char file_to_write[SIZE];
	sprintf(file_to_write, "tmp_download_station_%d_%d.txt", station_number, getpid());

    char line[SIZE];
    char fields[SIZE];

    // Ignore first and second lines
    fgets(line, SIZE, stream);
    fgets(line, SIZE, stream);
    // Field names (first line of file_to_write)
    fgets(fields, SIZE, stream);
    write_file(fields, file_to_write);

    char station[SIZE];
    sprintf(station, "%d", station_number);

    while (fgets(line, SIZE, stream))
    {
        char* tmp = strdup(line);
        // We get the first field of the line (the station number)
        char* field = (char*) getfield(tmp,1);

        if(!strcmp(field, station))
        {
        	line[strlen(line)-1] = '\0';
        	write_file(line, file_to_write);
        }
    }

    // send_file()
    // remove_file()
}

//-----------------------------------------------------------------------------------------------------------------

void daily()
{

}

//-----------------------------------------------------------------------------------------------------------------

void monthly()
{

}

//-----------------------------------------------------------------------------------------------------------------

void average()
{

}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Recieves a line and writes it to file
 * @param line string to write to file
 */
void write_file(char* line, const char* file_name)
{
    FILE *log = NULL;

    static int flag = 1;
    if (flag == 1)
    {
      fclose(fopen(file_name, "w"));
      flag = 0;
    } 

    log = fopen(file_name, "a"); 

    if (log == NULL)
    {
        printf("Error! can't open file %s", file_name);
    }
    fprintf(log, "%s\n", line);
    fclose(log);
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Given a line of fields separated by a comma, this function obtains the field in the specified position
 * @param line is the line of comma-separated values
 * @param num is the position where the desired field is in the line
 */
const char* getfield(char* line, int num)
{
    const char* tok;
    for (tok = strtok(line, ",");
            tok && *tok;
            tok = strtok(NULL, ","))
    {
        if (!--num)
            return tok;
    }
    return NULL;
}

//-----------------------------------------------------------------------------------------------------------------