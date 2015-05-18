/**
 * @file
 * @brief This file implements the storage server.
 *
 * The storage server should be named "server" and should take a single
 * command line argument that refers to the configuration file.
 *
 * The storage server should be able to communicate with the client
 * library functions declared in storage.h and implemented in storage.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include "utils.h"

#define MAX_LISTENQUEUELEN 20	///< The maximum number of queued connections.

TableHeader record_tables;  //declaration
pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER; //lock for threads

/**
 * @brief Process a command from the client.
 *
 * @param sock The socket connected to the client.
 * @param cmd The command received from the client.
 * @return Returns 0 on success, -1 otherwise.
 */

int authentication_check(struct config_params *p,char* username_cmp,char* password_cmp){
     if(strcmp(username_cmp,p->username)==0){
        if(strcmp(password_cmp,p->password)==0){
           return 0;
        }
     }
     return -1;
}

int checkColumns (Table *table, char* setrecord)
{
    char table_parameters[MAX_CONFIG_LINE_LEN];
    strcpy(table_parameters,table->columns);    ///Copy of table->columns, so we don't modify it.

    char* col = strtok(table_parameters," "); ///Number of columns.
    int num = atoi (col) , x , y;

    if (num == 0)       ///If there is no columns we just return and save the value.
        return 0;


    char *columns[MAX_COLUMNS_PER_TABLE+1];   ///Array for name of columns from user
    char *colValue[MAX_COLUMNS_PER_TABLE+1];  ///Array for values of columns from user


    char *table_columns[MAX_COLUMNS_PER_TABLE]; ///Array for name of columns from server
    char *table_colValue[MAX_COLUMNS_PER_TABLE];///Array for type of columns from server
    ///For loop to create the arrays for the names and types of the columns, coming from the server.
    for(x = 0 ; x < num ; x++) {
        table_columns[x] = strtok (NULL," ");   ///First string is the name.
        table_colValue[x] = strtok (NULL,",");  ///Second is the type
    }
    ///Parsing the users data into the arrays
    columns [0] = strtok(setrecord, " ");   ///First name of column
    colValue[0] = strtok(NULL,",");         ///First value of column
    for (x = 0 ; columns[x] != NULL ;){ ///While strtok doesn't return NULL.
        x++;                    ///If last 'columns' assignment was succesful we add 1 to x.
        if (x > num)            ///If there is more columns than it should.
            return -1;
        else {
            columns[x] = strtok(NULL," ");  ///Name of column from user
            colValue[x] = strtok(NULL,","); ///Value of column from user
        }
    }

    if (x < num)    ///In case there are more columns than it should.
        return -1;

    ///Comparing values parsed.
    for(x=0 ; x < num ; x++) {
        ///Compare to see if the name of the column it's the same on the server and on the client.
        if (strcmp(table_columns[x],columns[x]) != 0)
            return -1;

        ///Compare the value of the column (specified by the user) to see if it fits the right type.
                ///Integer
        if (atoi(table_colValue [x]) == 0){
            colValue[x] = strtok(colValue[x]," ");  ///Eliminates all spaces from the begining.
            if(strtok(NULL , " ") != NULL)          ///If there is still characters after the space, and thei're not spaces, return -1.
                return -1;
            for (y = 0; colValue[x][y] != '\0' ; y++){  ///Searching all characters until the end of string.
                if(!isdigit(colValue[x][y])){           ///If not digit fulfill condition.
                    if( !(y == 0 && (colValue[x][y] == '+' || colValue[x][y] == '-')) ) ///If the first digit is not '+' nor '-'.
                        return -1;
                }
            }
        }
                ///String
        else {
            while(colValue[x][0] == ' '){   ///If first character is a space, pointer to the first character will move to the next
                colValue[x]++;              ///character, and this process goes on until there is no white space on the begining.
            }
            for(y = 0 ; y < strlen(colValue[x]) ; y++){
                if(colValue[x][y] == ' ' && colValue[x][y+1] == '\0') {
                    colValue[x][y] = '\0';  ///If the last character is a space, remove it
                    y = 0;                  ///and start the loop again to check for other spaces in the end.
                }
            }
            if (strlen(colValue[x]) > (atoi(table_colValue [x]) - 1)) ///If string bigger than should be. (+ 1 because it includes the null-character)
                return -1;
        }
    }

    return 0;
}

double elapsedTime = 0;

int handle_command(int sock, char *cmd, struct config_params *p, TableHeader* record_tables)
{
        char *username_str;
        char *password_str;
        char *gettable_str;
        char *getkey_str;
        char *settable_str;
        char *setkey_str;
        char *setrecord_str;
        char *metada;
        char feedback[800];
        char message[MAX_CONFIG_LINE_LEN];
        struct timeval t1, t2;
        gettimeofday(&t1, NULL);
        sprintf(message , "Processing command '%s'\n", cmd);
        char *cmd_str=strtok(cmd," ");
        //Authentication
        if (strcmp(cmd_str,"AUTH")==0){
		username_str=strtok(NULL," ");
          	password_str=strtok(NULL," ");
          	p->status_auth = authentication_check(p,username_str,password_str);
          	if (p->status_auth == -1) {
            		sprintf(feedback , "error %d" , ERR_AUTHENTICATION_FAILED); //sends an error message with the number of the error.
          	}
          	else
            		sprintf(feedback , "ok");   //If the authentication was succesfull it sends to the client library an "ok".
        }
        //Get
        else if (strcmp(cmd_str,"GET")==0){
          	if(p->status_auth == 0 ) {
            		gettable_str=strtok(NULL," ");
            		getkey_str=strtok(NULL," ");
	    		if(table_parser(gettable_str) == 0 && key_parser(getkey_str) == 0) {
            			Table* tmp;
            			tmp = find_table(gettable_str,record_tables);
            			if (tmp == NULL)
                			sprintf(feedback , "error %d" , ERR_TABLE_NOT_FOUND);   //If it couldn't find the table it sends an error.
            			else {
                			Key* temp;
                			temp = find_key(getkey_str,tmp);
                			if (temp == NULL)
                                sprintf(feedback , "error %d" , ERR_KEY_NOT_FOUND); //If it couldn't find the key it sends an error.
                			else {
                                unsigned char inRec = *(temp->record.metadata);
                                if(p->concurrency == 0)         //If there is no concurrency, there is no metada to be compared.
                                    inRec = 0;
                                sprintf(feedback , "%d %s", inRec , temp->record.value);
                			}
            			}
	    		}
	    		else
                    sprintf(feedback , "error %d" , ERR_INVALID_PARAM);
        	}
          	else
                sprintf(feedback , "error %d" , ERR_NOT_AUTHENTICATED); //If not authenticated it sends an error.
        }
        //Set
        else if (strcmp(cmd_str,"SET")==0){
          	if(p->status_auth == 0){
            		settable_str=strtok(NULL," ");
            		setkey_str=strtok(NULL," ");
            		metada = strtok(NULL," ");
            		setrecord_str=strtok(NULL,"");
            		erase_space_ends(setrecord_str);
            		if (table_parser(settable_str) == 0 && key_parser(setkey_str) == 0 && value_parser(setrecord_str) == 0){
                        struct storage_record value;
                		strcpy(value.value , setrecord_str);
                		Table* tmp;

                		if(p->concurrency == 1){
                            pthread_mutex_lock(&locker);//mutex lock
                        }

                		tmp = find_table(settable_str,record_tables);
                		if (tmp == NULL)
                    			sprintf(feedback , "error %d" , ERR_TABLE_NOT_FOUND);
                		else {
                            //Check the string to see if the columns and values are valid
                            if (checkColumns(tmp, setrecord_str) != 0 && strcmp(setrecord_str , "NULL") != 0) {
                                sprintf(feedback , "error %d" , ERR_INVALID_PARAM);
                            }
                            else {
                                Key* temp;
                    			temp = find_key(setkey_str,tmp);
                    			if (temp == NULL){
                                    if (strcmp(setrecord_str , "NULL") == 0)
                                        sprintf(feedback , "error %d" , ERR_KEY_NOT_FOUND);
                                    else {
                                        if(tmp->numRec < MAX_RECORDS_PER_TABLE) {
                                            add_key(setkey_str, value, tmp);

                                            if(p->concurrency == 1)
                                                pthread_mutex_unlock(&locker);//unlock

                                            strcpy(feedback , "added 1");         //sends back that is adding a new key.
                                        }
                                        else
                                            sprintf(feedback , "error %d" , ERR_UNKNOWN);
                                    }
                    			}
                    			else if (strcmp(setrecord_str , "NULL") == 0){
                        			delete_key(temp,tmp);

                        			if(p->concurrency == 1)
                                        pthread_mutex_unlock(&locker);//unlock

                        			strcpy(feedback , "del");           //sends back that is deleting a key.
                    			}
                    			else{
                                    unsigned char inRec = *(temp->record.metadata);
                                    if(inRec == atoi(metada) || atoi(metada) == 0) {   ///If record sent from client has same metada of record from server.
                                        strcpy(temp->record.value , value.value);
                                        inRec++;
                                        *(temp->record.metadata) = inRec;

                                        if(p->concurrency == 1)
                                            pthread_mutex_unlock(&locker);//unlock

                                        sprintf(feedback , "updt %d",inRec);          //sends back that is updating a key.
                                    }
                                    else
                                        sprintf(feedback , "error %d" , ERR_TRANSACTION_ABORT);
                    			}
                            }
                        }
                        if(p->concurrency == 1)
                            pthread_mutex_unlock(&locker);//unlock in case it wasn't unlocked
            		}
            		else
                		sprintf(feedback , "error %d" , ERR_INVALID_PARAM); //If key or value are invalid parameters it sends an error.
            }
          	else
                sprintf(feedback , "error %d" , ERR_NOT_AUTHENTICATED);
        }
        ///Query
        else if(strcmp(cmd_str,"QUE")==0){
            if(p->status_auth == 0){
                char *table;
                table = strtok(NULL , " ");
                Table *temp;
                temp = find_table(table , record_tables);
                if(temp != NULL){
                    char *predicates_line;
                    predicates_line = strtok(NULL,"");  ///Copy of the current line that wasn't parsed.
                    strcpy(feedback , query_parsing(temp , predicates_line));
                }
                else    ///Table not find
                    sprintf(feedback , "error %d" , ERR_TABLE_NOT_FOUND);
            }
            else    ///Not authenticated.
                sprintf(feedback , "error %d" , ERR_NOT_AUTHENTICATED);
        }

        if (LOGGING == 1 || LOGGING == 2)
            logger(log_file , message);

    	gettimeofday(&t2, NULL);
    	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;
    	elapsedTime += (t2.tv_sec - t1.tv_sec);
    	//printf("%.5f - Server\n" , elapsedTime);  ///Used for measuring the performance.
        // For now, just send back the command to the client.
        sendall(sock, feedback, strlen(feedback));
        sendall(sock, "\n", 1);

	return 0;
}

void *serve_client (void *argth){
    int sock;
    struct config_params params;
    char message[MAX_CONFIG_LINE_LEN];
    char clientAddr[40];
    int clientPort;

    sock = ((ArgThread*) argth)->clientsock;
    strcpy(params.username , ((ArgThread*) argth)->parameters_server.username);
    strcpy(params.password , ((ArgThread*) argth)->parameters_server.password);
    params.status_auth = ((ArgThread*) argth)->parameters_server.status_auth;
    params.concurrency = ((ArgThread*) argth)->parameters_server.concurrency;
    strcpy(clientAddr , ((ArgThread*) argth)->client_addr);
    clientPort = ((ArgThread*) argth)->client_port;

    if(params.concurrency == 1)
        pthread_detach(pthread_self());

    free(argth);

    int wait_for_commands = 1;
    do {
        // Read a line from the client.
        char cmd[MAX_CMD_LEN];
        int status = recvline(sock, cmd, MAX_CMD_LEN);
        if (status != 0) {pthread_mutex_unlock(&locker);//unlock
            // Either an error occurred or the client closed the connection.
            wait_for_commands = 0;
        } else {
            // Handle the command from the client.
            int status = handle_command(sock, cmd, &params, &record_tables);
            if (status != 0)
                wait_for_commands = 0; // Oops.  An error occured.
        }
    } while (wait_for_commands);

    // Close the connection with the client.
    close(sock);

    sprintf(message , "Closed connection from %s:%d.\n", clientAddr , clientPort);

    if (LOGGING == 1 || LOGGING == 2)
        logger(log_file , message);

    return NULL;
}

/**
 * @brief Start the storage server.
 *
 * This is the main entry point for the storage server.  It reads the
 * configuration file, starts listening on a port, and proccesses
 * commands from clients.
 */
int main(int argc, char *argv[])
{
	char time_log[25] , name[40], message[80];
	// Process command line arguments.
	// This program expects exactly one argument: the config file name.
	assert(argc > 0);
	if (argc < 2) {
		printf("Usage %s <config_file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	char *config_file = argv[1];

	// Read the config file.
	struct config_params params;
	record_tables.headTable = NULL; //Initializing the record
	record_tables.numTable = 0;     //to make sure.
	int status = read_config(config_file, &params, &record_tables);
	params.status_auth = -1;
	if (status != 0) {
        free_memory(&record_tables);
		printf("Error processing config file.\n");
		exit(EXIT_FAILURE);
	}

    ///This is the function we used for loading the 'census' file. It's used as a second (or more) parameter and is not strictly necessary.
	if(argc > 2) {
        int y;
        for(y = 2 ; y < argc ; y++) {
            FILE* pfile = fopen( argv[y] , "r");    ///Opens the file
            Table* tmp;
            Key* temp;
            tmp = find_table(argv[y] , &record_tables);
            if (tmp != NULL){                       ///See if the table (name of the file) exists.
                if (pfile != NULL) {
                    char c = 50;
                    char key[MAX_KEY_LEN] , value[MAX_VALUE_LEN];
                    while (c != EOF) {
                        fscanf (pfile , "%s %s" , key , value);     ///Read values from file.
                        temp = find_key(key , tmp);
                        struct storage_record record;
                        strcpy(record.value , value);
                        if(temp == NULL)
                            add_key(key , record  , tmp);           ///If the key doesn't exist it creates a new one.
                        else
                            strcpy(temp->record.value , value);     ///If the key exists it updates the key.
                        c = fgetc(pfile);
                    }
                }
            }
        }
	}

	generate_time_string_log(time_log);
	sprintf(name , "Server-%s", time_log );
	sprintf(message , "Server on %s:%d\n", params.server_host, params.server_port);

	if (LOGGING == 2) {
		log_file = fopen( name , "w" );
		if (log_file == NULL)
			printf("SERVER: An error occured genereting the log file\n");
	}

	else if (LOGGING == 1)
		log_file = stdout;

	if (LOGGING == 1 || LOGGING == 2)
		logger(log_file , message);

	erase_string(message);


	// Create a socket.
	int listensock = socket(PF_INET, SOCK_STREAM, 0);
	if (listensock < 0) {
        free_memory(&record_tables);
		printf("Error creating socket.\n");
		exit(EXIT_FAILURE);
	}

	// Allow listening port to be reused if defunct.
	int yes = 1;
	status = setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
	if (status != 0) {
        free_memory(&record_tables);
		printf("Error configuring socket.\n");
		exit(EXIT_FAILURE);
	}

	// Bind it to the listening port.
	struct sockaddr_in listenaddr;
	memset(&listenaddr, 0, sizeof listenaddr);
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_port = htons(params.server_port);
	inet_pton(AF_INET, params.server_host, &(listenaddr.sin_addr)); // bind to local IP address
	status = bind(listensock, (struct sockaddr*) &listenaddr, sizeof listenaddr);
	if (status != 0) {
        free_memory(&record_tables);
		printf("Error binding socket.\n");
		exit(EXIT_FAILURE);
	}

	// Listen for connections.
	status = listen(listensock, MAX_LISTENQUEUELEN);
	if (status != 0) {
        free_memory(&record_tables);
		printf("Error listening on socket.\n");
		exit(EXIT_FAILURE);
	}

	// Listen loop.
	int wait_for_connections = 1;
	pthread_t tip;
	while (wait_for_connections) {
		// Wait for a connection.
		params.status_auth = -1;        ///Every time it starts listening for connections it "de-authenticates" the server.
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof clientaddr;
		int clientsock = accept(listensock, (struct sockaddr*)&clientaddr, &clientaddrlen);
		if (clientsock < 0) {
			printf("Error accepting a connection.\n");
			free_memory(&record_tables);
			exit(EXIT_FAILURE);
		}

		sprintf(message , "Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);

		if (LOGGING == 1 || LOGGING == 2)
			logger(log_file , message);
		erase_string(message);

		ArgThread *argth = (ArgThread*)malloc(sizeof(ArgThread));
		argth->clientsock = clientsock;
		strcpy(argth->parameters_server.username , params.username);
		strcpy(argth->parameters_server.password , params.password);
		argth->parameters_server.status_auth = params.status_auth;
		argth->parameters_server.concurrency = params.concurrency;
		strcpy(argth->client_addr , inet_ntoa(clientaddr.sin_addr));
		argth->client_port = clientaddr.sin_port;

		// Get commands from client.
        if(params.concurrency == 0)
            serve_client((void*) argth);
        else if(params.concurrency == 1)
            pthread_create( &tip , NULL , serve_client , (void*) argth);
        else if(params.concurrency == 2) {
            serve_client((void*) argth);
        }
        else if(params.concurrency == 3) {
            pid_t child;
            if((child = fork()) == 0) {
                close(listensock);
                serve_client((void *) argth);
                exit(0);
            }
            free(argth);
        }

	}

	// Stop listening for connections.
	close(listensock);
	if (LOGGING == 2)
		fclose(log_file);
    free_memory(&record_tables);
    //printf("%.5f - Server\n" , elapsedTime);
	return EXIT_SUCCESS;
}


