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
#include <arpa/inet.h>
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
 * Information for connection oriented socket
 */
int c_server_socket, c_new_client_socket;
struct sockaddr_in server_struct, client_struct;
int port = 6020;
int client_length = sizeof(client_struct);

/*
 * Information for non-connection oriented socket
 */
int nc_server_socket;
struct sockaddr_in nc_client_socket;

/*
 * Functions to establish and keep connection
 */
void create_socket();
void create_nc_socket();
void listen_socket();
void login();
void process_command(const char*, char*);
void create_socket_n(); // Non oriented to conection

/*
 * Functions to execute commands sent by client
 */
void list();
void download(int);
void daily(int);
void monthly(int);
void average(const char*); 

/*
 * Function to send the generated file
 */
void write_file(char*, const char*);
void send_file(const char*);

/*
 * Auxiliary functions
 */
const char* getfield(char*, int);


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
/*
			n = write(c_new_client_socket, buffer_out, 100);
			if ( n < 0 ) 
			{
				perror(" > Error while sendind data \n > Exiting \n");
				exit(1);
			}
			*/
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
		printf(" > Recieved command <listar> \n");	
		list();
		strcpy(buffer_out, "");
	}
	else  if(!strcmp("descargar", buffer))
	{
		// Let's extract station number
		aux = strtok(NULL, "\0");
		int station = -1;
		if(aux != NULL)
		station = atoi(aux);
		printf(" > Recieved command <descargar> for station %d \n", station);	
		download(station);	
	}
	else  if(!strcmp("diario_precipitacion", buffer))
	{
		// Let's extract station number
		aux = strtok(NULL, "\0");
		int station = -1;
		if(aux != NULL)
		station = atoi(aux);
		printf(" > Recieved command <diario_precipitacion> for station %d \n", station);
		daily(station);		
	}
	else  if(!strcmp("mensual_precipitacion", buffer))
	{
		// Let's extract station number
		aux = strtok(NULL, "\0");
		int station = -1;
		if(aux != NULL)
		station = atoi(aux);
		printf(" > Recieved command <mensual_precipitacion> for station %d \n", station);	
		monthly(station);
	}
	else  if(!strcmp("promedio", buffer))
	{
		// Let's extract the variable
		aux = strtok(NULL, "\0");
		char variable[SIZE];
		if(aux != NULL)
		strcpy(variable, aux);
		printf(" > Recieved command <promedio> for variable <%s> \n", variable);
		average(variable);
	}
	else
	{
		printf(" > Unknown command recieved \n");
		int n = write(c_new_client_socket, "unknown", SIZE);
		n = n;
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

	char file_to_write[SIZE];
	sprintf(file_to_write, "tmp_list_%d.txt", getpid());

    char line[SIZE];
    char fields[SIZE];

    // Ignore first and second lines
    fgets(line, SIZE, stream);
    fgets(line, SIZE, stream);
    // Field names
    fgets(fields, 1024, stream);

    char last_station_found[SIZE]= "rubbish";

    //int n = write(c_new_client_socket, " Stations:", 1024);

    while (fgets(line, SIZE, stream))
    {
        char* tmp = strdup(line);
        char* field = (char*) getfield(tmp,2);

        /*
         * If we still haven't found this station, we add this station to the file
         */
        if(strcmp(field, last_station_found))
        {
          //char line_to_write[SIZE] = "";
          char aux[SIZE] = "";

          strcpy(last_station_found, field);
          char station_information[SIZE];
          strcat(station_information, field);

          sprintf(aux, "%s", last_station_found);
          write_file(aux, file_to_write);
          //sprintf(aux, "   - %s", last_station_found);
          //n = write(c_new_client_socket, aux, SIZE);

          int i = 5;
          while(i < 20) // We analyze all fields for this station
          {
            tmp = strdup(line);
            field = (char*) getfield(tmp,i);

            if(strcmp(field, "--")) // We check if the field is completed or not
            {   
	              char* t = strdup(fields);
	              char* field_name = (char*) getfield(t,i);

	              sprintf(aux, "  - %s", field_name);
          		  write_file(aux, file_to_write);
				  //sprintf(aux, "       %s", field_name);
	              //n = write(c_new_client_socket, aux, SIZE);

	              //sprintf(aux, "%s - ", field_name);
	              //strcat(line_to_write, aux);              
            }
            i++;
          }
        }
        free(tmp);
    }
    //n = write(c_new_client_socket, "end", SIZE);
    send_file(file_to_write);
    // Delete tmp file
    unlink(file_to_write);
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Generates a file with ALL the information for the specified station number
 * @param station_number is the number of the station sent by the client
 */
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

    int found = 0;

    while (fgets(line, SIZE, stream))
    {
        char* tmp = strdup(line);
        // We get the first field of the line (the station number)
        char* field = (char*) getfield(tmp,1);

        if(!strcmp(field, station))
        {
        	found = 1;
        	line[strlen(line)-1] = '\0';
        	write_file(line, file_to_write);
        }
    }

    if(found)
    {
    	send_file(file_to_write);
    	// Delete tmp file
    	unlink(file_to_write);
    }
    else
    	write(c_new_client_socket, "unknown", SIZE);
    
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Generates a file with the daily sumation of the "precipitacion" field for the specified station
 * @param station_number is the number of the station sent by the client
 */
void daily(int station_number)
{
	FILE* stream = fopen("datos_meteorologicos_s.CSV", "r");
	char file_to_write[SIZE];
	sprintf(file_to_write, "tmp_daily_precipitacion_%d_%d.txt", station_number, getpid());

    char line[SIZE];
    char fields[SIZE];

    // Ignore first and second lines
    fgets(line, SIZE, stream);
    fgets(line, SIZE, stream);
    // Field names (first line of file_to_write)
    fgets(fields, SIZE, stream);
    sprintf(fields, "Numero Estacion,Fecha,Acumulacion precipitacion");
    write_file(fields, file_to_write);

    char station[SIZE];
    sprintf(station, "%d", station_number);

    int last_found_day = 1;
    double sum = 0;

    char previous_date[SIZE];

    int found = 0;

    while (fgets(line, SIZE, stream))
    {
        char* tmp = strdup(line);
        // We get the first field of the line (the station number)
        char* field = (char*) getfield(tmp,1);

        if(!strcmp(field, station))	// If we're in the write station
        {
        	found = 1;

        	line[strlen(line)-1] = '\0';

        	// We get the first two chars of the date (for example '01' for day 01)
        	tmp = strdup(line);
        	char* date = (char*) getfield(tmp,4);
        	char day[32];
        	strncpy(day, date, 2);
			day[2] = '\0';

			if(last_found_day == 1) // Only the first time, let's save the date
			{
				char previous_date[SIZE];
				strcpy(previous_date, date);
			}

			if(last_found_day == atoi(day)) // We're already on this day, let's keep adding
			{
				tmp = strdup(line);
        		char* precipitacion = (char*) getfield(tmp,8);
        		double p = 0;
        		sscanf(precipitacion, "%lf", &p);
        		sum += p;
        		strcpy(previous_date, date);
			}
			else // We found a new day, let's write the last one to the file and reset the sum
			{
				char line_out[SIZE];
				sprintf(line_out, "%d,%s,%lf", station_number, strtok(previous_date," "), sum);
				write_file(line_out, file_to_write);
				sum = 0;
				last_found_day = atoi(day);

				// We must keep in count the first time we found this day but adding it's precipitation
				//char* precipitacion = (char*) getfield(tmp,8);
        		//double p = 0;
        		//sscanf(precipitacion, "%lf", &p);
        		//sum += p;
			}
        }
    }

    // We must write the last day after we break the while
    if(last_found_day != 1)
    {
    	char line_out[SIZE];
    	sprintf(line_out, "%d,%s,%lf", station_number, strtok(previous_date," "), sum);
		write_file(line_out, file_to_write);
    }
   
   	if(found)
    {
    	send_file(file_to_write);
    	// Delete tmp file
    	unlink(file_to_write);
    }
    else
    	write(c_new_client_socket, "unknown", SIZE);
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Generates a file with the monthly sumation of the "precipitacion" field for the specified station
 * @detail We assume that the csv file will have data for only ONE month, not several
 * @param station_number is the number of the station sent by the client
 */
void monthly(int station_number)
{
	FILE* stream = fopen("datos_meteorologicos_s.CSV", "r");
	char file_to_write[SIZE];
	sprintf(file_to_write, "tmp_monthly_precipitacion_%d_%d.txt", station_number, getpid());

    char line[SIZE];
    char fields[SIZE];

    // Ignore first and second lines
    fgets(line, SIZE, stream);
    fgets(line, SIZE, stream);
    // Field names (first line of file_to_write)
    fgets(fields, SIZE, stream);
    // We overwrite the fields
    sprintf(fields, "Numero Estacion,Mes,Acumulacion precipitacion");
    write_file(fields, file_to_write);

    char station[SIZE];
    sprintf(station, "%d", station_number);

	int month = -1;
    double sum = 0;

    int found = 0;

    while (fgets(line, SIZE, stream))
    {
        char* tmp = strdup(line);
        // We get the first field of the line (the station number)
        char* field = (char*) getfield(tmp,1);

        if(!strcmp(field, station))	// If we're in the write station
        {
        	found = 1;
        	
         	// We save the current month
        	tmp = strdup(line);
        	char* date = (char*) getfield(tmp,4);
        	date[5] = '\0';
        	month = atoi(date+3);  	

        	// We get the precipitation to add it to sum
        	tmp = strdup(line);
        	char* precipitacion = (char*) getfield(tmp,8);
        	double p = 0;
        	sscanf(precipitacion, "%lf", &p);
        	sum += p;
		}
    }
    char line_out[SIZE];
    sprintf(line_out, "%d,%02d,%lf", station_number, month, sum);
    if(month != -1)
		write_file(line_out, file_to_write);

    if(found)
    {
    	send_file(file_to_write);
    	// Delete tmp file
    	unlink(file_to_write);
    }
    else
    	write(c_new_client_socket, "unknown", SIZE);
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Calculates the average of the specified variable for every station on the file
 * @param var the name of the variable whose average we need
 */
void average(const char* var)
{
	/*
	 * We define the position of each field in the CSV line
	 */
	#define temperatura 	5
	#define humedad 		6
	#define punto			7
	#define precipitacion 	8	
	#define velocidad		9
	#define direccion 		10
	#define rafaga			11
	#define presion			12	
	#define radiacion		13
	#define suelo			14

	int VARIABLE_INDEX = -1;
	if(!strcmp(var, "temperatura"))
		VARIABLE_INDEX = temperatura;
	else if(!strcmp(var, "humedad"))
		VARIABLE_INDEX = humedad;
	else if(!strcmp(var, "punto de rocio"))
		VARIABLE_INDEX = punto;
	else if(!strcmp(var, "precipitacion"))
		VARIABLE_INDEX = precipitacion;
	else if(!strcmp(var, "velocidad viento"))
		VARIABLE_INDEX = velocidad;
	else if(!strcmp(var, "direccion viento"))
		VARIABLE_INDEX = direccion;
	else if(!strcmp(var, "rafaga maxima"))
		VARIABLE_INDEX = rafaga;
	else if(!strcmp(var, "presion"))
		VARIABLE_INDEX = presion;
	else if(!strcmp(var, "radiacion solar"))
		VARIABLE_INDEX = radiacion;
	else if(!strcmp(var, "temperatura suelo 1"))
		VARIABLE_INDEX = suelo;
	else
	{
		printf(" > Average command for unknown variable <%s> \n", var);
		int n = write(c_new_client_socket, "unknown", SIZE);
		n = n;
		return;
	}

	FILE* stream = fopen("datos_meteorologicos_s.CSV", "r");

	char file_to_write[SIZE];
	sprintf(file_to_write, "tmp_average_%s_%d.txt", var, getpid());

    char line[SIZE];
    char fields[SIZE];

    // Ignore first and second lines
    fgets(line, SIZE, stream);
    fgets(line, SIZE, stream);
    // Field names
    fgets(fields, SIZE, stream);
    sprintf(fields, "Numero Estacion,Promedio %s", var);
    write_file(fields, file_to_write);

    int last_station_found = 30135; // First station

    double sum = 0;
    double count = 0;

    while (fgets(line, SIZE, stream))
    {
        char* tmp = strdup(line);
        char* field = (char*) getfield(tmp,1);

        /*
         * If we're still analyzing the same station, we add to the sum variable and increment the counter
         */
        if(last_station_found == atoi(field))
        {
          	tmp = strdup(line);
        	char* variable_value = (char*) getfield(tmp,VARIABLE_INDEX);
        	double value = 0;
        	sscanf(variable_value, "%lf", &value);
        	sum += value;
        	count++;
        }
        else // If we found a new station, then we need to printf for the prevois one
        {
        	/*
        	 * We write to file
        	 */
        	char line_out[SIZE];
        	sprintf(line_out, "%d,%lf", last_station_found, sum/count);
        	write_file(line_out, file_to_write);

        	/*
        	 * We reset the variables
        	 */
        	sum = 0;
        	count = 0;
        	last_station_found = atoi(field);
        }
        free(tmp);
    }

    /*
     * We need to write the file for the last station (as the while was broken)
     */
    char line_out[SIZE];
    sprintf(line_out, "%d,%lf", last_station_found, sum/count);
    write_file(line_out, file_to_write);

    send_file(file_to_write);
    // Delete tmp file
    unlink(file_to_write);
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
        printf(" >  Error when opening file %s\n", file_name);
    }
    fprintf(log, "%s\n", line);
    fclose(log);
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Uses a non-connection oriented socket to send the specified file
 * @param file_name name of the file to be sent
 */
void send_file(const char* file_name)
{
	/*
	 * We establish the connection through a non-connection oriented socket
	 */
	create_nc_socket();

	char buffer[SIZE];
	int address_size = sizeof(nc_client_socket);
	int n;

	printf(" > Sending file %s\n", file_name);

	/*
	 * We open the file and send every line, one by one
	 */
	FILE *file = fopen(file_name, "r");
	while(fgets(buffer, SIZE, file) != NULL)
	{
		// We send a single line
		n = sendto(nc_server_socket, (void*) buffer, SIZE, 0, (struct sockaddr*) &nc_client_socket, sizeof(nc_client_socket));
		if (n < 0) 
		{
			perror(" > Error while writing socket \n > Exiting\n");
			exit(1);
		}
		// We clean the buffer
		memset(buffer, '\0', sizeof(buffer));

		// We wait for a ACK reply in order to keep sending
		n = recvfrom(nc_server_socket, (void*) buffer, SIZE, 0, (struct sockaddr*) &nc_client_socket, (socklen_t*) &address_size);
		if (n < 0) 
		{
			perror(" > Error while reading socket \n > Exiting\n");
			exit(1);
		}
		memset(buffer, 0, SIZE);
	}
	fclose(file);

	/*
	 * We send a message to indicate the transfer has ended
	 */
	address_size = sizeof(nc_client_socket);
	n = sendto(nc_server_socket, (void *)"end", 4, 0, (struct sockaddr*) &nc_client_socket, address_size);
	if ( n < 0 ) 
	{
		perror(" > Error while writing socket \n >Exiting\n");
		exit(1);
	}

	memset(buffer, 0, sizeof(buffer));
	n = recvfrom(nc_server_socket, (void*) buffer, SIZE, 0, (struct sockaddr*) &nc_client_socket, (socklen_t*) &address_size);
	if ( n < 0 ) 
	{
		perror(" > Error while reading socket \n > Exiting\n");
		exit(1);
	}
	close(nc_server_socket);
}

//-----------------------------------------------------------------------------------------------------------------

/*
 * @brief Creates a non-connection oriented socket 
 */
void create_nc_socket()
{
	//c_new_client_socket = socket(AF_INET, SOCK_STREAM, 0);
	/*
	 * We inform that the data is about to be sent
	 */
    int n = write(c_new_client_socket, "begin transfer", SIZE);
	if (n < 0)
	{
		perror(" > Error while writing to socket \n > Exiting\n");
		exit(1);
    }
	
	/*
	 * We ask the client to provide us with the port to make the transfer
	 */				
	char non_connection_port[16];
	memset(non_connection_port, '\0', 16);
	n = read(c_new_client_socket, non_connection_port, 15);
	if ( n < 0 ) 
	{
		perror(" > Error while reading socket \n > Exiting\n");
		exit(1);
	}

	int nc_port = atoi(non_connection_port);
	//printf(" > File transfer for process %d is %d\n", getpid(), nc_port);					
							
	/*
	 * Let's establish the connection with the recieved port
	 */

	//int tamano_direccion, n;
	
	nc_server_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if (nc_server_socket < 0) 
	{
		perror(" > Error while creating to socket \n > Exiting\n");
		exit(1);
	}

	nc_client_socket.sin_family = AF_INET;
	nc_client_socket.sin_port = htons(nc_port);

	/*
	 * We find the host
	 */
	//char host[INET_ADDRSTRLEN];
	nc_client_socket.sin_addr = client_struct.sin_addr;
	//inet_ntop(AF_INET, &(nc_client_socket.sin_addr), host, INET_ADDRSTRLEN);

	printf(" > Ready to send file to process %d on port %d\n", getpid(), ntohs(nc_client_socket.sin_port));
	memset(&(nc_client_socket.sin_zero), '\0', 8);
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