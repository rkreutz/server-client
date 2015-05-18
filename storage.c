/**
 * @file
 * @brief This file contains the implementation of the storage server
 * interface as specified in storage.h.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include "storage.h"
#include "utils.h"


/**
 * @brief This is just a minimal stub implementation.  You should modify it
 * according to your design.
 */
void* storage_connect(const char *hostname, const int port)
{
	if (port < 1024 || port > 65535 || hostname == NULL) {
		errno = ERR_INVALID_PARAM;
		return NULL;
	}
	else if (strlen(hostname) > MAX_HOST_LEN) {
        errno = ERR_INVALID_PARAM;
		return NULL;
	}
	// Create a socket.
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0){
        errno = ERR_CONNECTION_FAIL;
		return NULL;
	}
	// Get info about the server.
	struct addrinfo serveraddr, *res;
	memset(&serveraddr, 0, sizeof serveraddr);
	serveraddr.ai_family = PF_INET;   ///AF_UNSPEC, modify this whe passing back to unix.
	serveraddr.ai_socktype = SOCK_STREAM;
	char portstr[MAX_PORT_LEN];
	snprintf(portstr, sizeof portstr, "%d", port);
	int status = getaddrinfo(hostname, portstr, &serveraddr, &res);
	if (status != 0){
        errno = ERR_CONNECTION_FAIL;
		return NULL;
	}

	// Connect to the server.
	status = connect(sock, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
        errno = ERR_CONNECTION_FAIL;
		return NULL;
	}
	char message[80];
	sprintf(message , "Connected to server %s:%d\n", hostname, port);

	if (LOGGING == 1 || LOGGING == 2)
		logger(log_file , message);	//Before it ends , if everything is okay, it will generate the log file, with the
						//description of what was made. I've placed it here because it's just before it returns '0'
						//(when everything went as planned). Notify to which server the client connected

	return (void*) sock;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it
 * according to your design.
 */
int storage_auth(const char *username, const char *passwd, void *conn)
{
	// Connection is really just a socket file descriptor.
	int sock = (int)conn;
    if (conn == NULL || username == NULL || passwd == NULL) {
        errno = ERR_INVALID_PARAM;
        return -1;
    }
	// Send some data.
	char buf[MAX_CMD_LEN];
	char message[MAX_CMD_LEN];
	char *parser;
	memset(buf, 0, sizeof buf);
	char *encrypted_passwd = generate_encrypted_password(passwd, NULL);
	snprintf(buf, sizeof buf, "AUTH %s %s\n", username, encrypted_passwd);
	sprintf(message , "Processing command 'AUTH %s %s'\n", username, encrypted_passwd);
	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
        parser = strtok(buf , " ");
        if (strcmp(parser , "error") == 0){     ///If it receives from the server an error it will return '-1'
            parser = strtok(NULL , " ");        ///and set errno to the apropriate error.
            errno = atoi(parser);
            return -1;
        }


		if (LOGGING == 1 || LOGGING == 2)
			logger(log_file , message);	//Before it ends , if everything is okay, it will generate the log file, with the
							//description of what was made. I've placed it here because it's just before it returns '0'
							//(when everything went as planned). Notify when and who logged into the server.
		return 0;
	}
	else {
        errno = ERR_CONNECTION_FAIL;
        return -1;
	}

    errno = ERR_UNKNOWN;
	return -1;
}

/**
 * @brief This is just a minimal stub implementation.  You should modify it
 * according to your design.
 */
int storage_get(const char *table, const char *key, struct storage_record *record, void *conn)
{
	// Connection is really just a socket file descriptor.
	int sock = (int)conn;
    if (conn == NULL || table == NULL || key == NULL || record == NULL) {
        errno = ERR_INVALID_PARAM;
        return -1;
    }
    if (table_parser(table) != 0) {
        errno = ERR_INVALID_PARAM;
        return -1;
    }
    if (key_parser(key) != 0) {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

	// Send some data.
	char buf[MAX_CMD_LEN];
	char message[MAX_CONFIG_LINE_LEN];
	char *parser;
	char *columns[MAX_COLUMNS_PER_TABLE],*value[MAX_COLUMNS_PER_TABLE];
	memset(buf, 0, sizeof buf);
	snprintf(buf, sizeof buf, "GET %s %s\n", table, key);
	sprintf(message , "Processing command 'GET %s %s'\n", table, key);
	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
	    parser = strtok(buf , " ");
            if (strcmp(parser , "error") == 0){     ///Processes the error case.
            	parser = strtok(NULL , " ");
            	errno = atoi(parser);
            	return -1;
            }
        unsigned char fromRec = atoi(parser);
        *(record->metadata) = fromRec;
        parser = strtok(NULL , "");
        strcpy(record->value , parser);

	    if (LOGGING == 1 || LOGGING == 2)
		logger(log_file , message);	//Before it ends, if everything is okay, it will generate the log file, with the
							//description of what was made. I've placed it here because it's just before it returns '0'
	    int x=0,y;					//(when everything went as planned). Monitoring client activity.
	    if(LOGGING == 1 || LOGGING == 2) {
            columns[0] = strtok(parser , ",");
            while(columns[x] != NULL) {
                x++;
                columns[x] = strtok(NULL , ",");
            }
            printf("Table: %s \t Key: %s\n",table,key);
            for(y = 0; y<x ; y++) {
                columns[y] = strtok(columns[y]," ");
                value[y] = strtok(NULL , "");
                erase_space_ends(columns[y]);
                erase_space_ends(value[y]);
                printf(" %-30s |",columns[y]);
            }
            printf("\n");
            for(y = 0; y<x ; y++)
                printf(" %-30s |",value[y]);
            printf("\n");
	    }

	    return 0;
	}
	else {
        errno = ERR_CONNECTION_FAIL;
        return -1;
	}

	errno = ERR_UNKNOWN;
	return -1;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it
 * according to your design.
 */
int storage_set(const char *table, const char *key, struct storage_record *record, void *conn)
{
	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

    if (conn == NULL || table == NULL || key == NULL ) {
        errno = ERR_INVALID_PARAM;
        return -1;
    }
    if (table_parser(table) != 0) {
        errno = ERR_INVALID_PARAM;
        return -1;
    }
    if (key_parser(key) != 0) {
        errno = ERR_INVALID_PARAM;
        return -1;
    }
    if (record != NULL){
        if (value_parser(record->value) != 0){
            errno = ERR_INVALID_PARAM;
            return -1;
        }
    }
	// Send some data.
	char buf[MAX_CMD_LEN];
	char message[MAX_CONFIG_LINE_LEN];
	char *parser;
	char meta_client;
	if(record != NULL)
        meta_client = *(record->metadata);
	memset(buf, 0, sizeof buf);
	if(record != NULL){
        snprintf(buf, sizeof buf, "SET %s %s %d %s\n", table, key, meta_client, record->value);
        sprintf(message , "Processing command 'SET %s %s %d %s'\n", table, key, meta_client, record->value);
	}
	else {
        snprintf(buf, sizeof buf, "SET %s %s 0 NULL\n", table, key);
        sprintf(message , "Processing command 'SET %s %s 0 NULL'\n", table, key);
	}
	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
		parser = strtok(buf , " ");
        if (strcmp(parser , "error") == 0){     ///Error case.
            parser = strtok(NULL , " ");
            errno = atoi(parser);
            return -1;
        }
        else if (strcmp(parser , "updt") == 0){     ///If it's updating the key it will say so.
            parser = strtok(NULL , " ");
            unsigned char fromRec = atoi(parser);
            //*(record->metadata) = fromRec;    //in case we want to change metadata.
            if (LOGGING != 0) printf("Succesfully updated key '%s' from table '%s'.\n",key ,table);
        }
        else if (strcmp(parser , "added") == 0){    ///If it's addind the key it will say so.
            parser = strtok(NULL , " ");
            unsigned char fromRec = atoi(parser);
            //*(record->metadata) = fromRec;    //in case we want to change metadata.
            if (LOGGING != 0) printf("Succesfully added key '%s' to table '%s'.\n",key ,table);
        }
        else if (strcmp(parser , "del") == 0) {     ///If it's deleting the key it will say so.
            if (LOGGING != 0) printf("Succesfully deleted key '%s' from table '%s'.\n",key ,table);
        }
		if (LOGGING == 1 || LOGGING == 2)
			logger(log_file , message);	//Before it ends , if everything is okay, it will generate the log file, with the
							//description of what was made. I've placed it here because it's just before it returns '0'
							//(when everything went as planned). Notify what was changed inside the server by the client.
		return 0;
	}
	else {
        errno = ERR_CONNECTION_FAIL;
        return -1;
	}

    errno = ERR_UNKNOWN;
	return -1;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it
 * according to your design.
 */
int storage_disconnect(void *conn)
{
	// Cleanup
    if (conn == NULL) {
        errno = ERR_INVALID_PARAM;
     	return -1;
    }
	int x;
	int sock = (int)conn;
	x = close(sock);
	if (x != 0){
        errno = ERR_UNKNOWN;
        return -1;
	}
			//should have a logger, but as we were asked to add just 4 loggers
			//and I thought this command is the less important to be logged
			// it wont have a logger.
	return 0;
}

int storage_query(const char *table, const char *predicates, char **keys, const int max_keys, void *conn) {
    int sock = (int)conn;

    if (conn == NULL || table == NULL || keys == NULL || predicates == NULL) {
        errno = ERR_INVALID_PARAM;
        return -1;
    }
    if (table_parser(table) != 0) {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    char buf[MAX_CMD_LEN],backup[MAX_CMD_LEN];
    char message[MAX_CONFIG_LINE_LEN];
    char *parser;
    memset(buf, 0, sizeof buf);
    snprintf(buf, sizeof buf, "QUE %s %s\n", table, predicates);
    sprintf(message , "Processing command 'QUE %s %s\n", table, predicates);
    if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
        if(iscntrl(*buf) == 0) {
            strcpy(backup , buf);
            parser = strtok(buf , " ");
            if (strcmp(parser , "error") == 0){     ///Processes the error case.
                parser = strtok(NULL , " ");
                errno = atoi(parser);
                return -1;
            }
        }
        int x = 0;
        if(iscntrl(*buf) == 0) {
            keys[0] = strtok(backup , " \n");
            for(x = 0 ; x < max_keys && keys[x] != NULL ; ){
                x++;
                keys[x] = strtok(NULL , " \n");
            }
            if(keys[x] != NULL){
                x++;
                while(strtok(NULL , " \n\0") != NULL)
                    x++;
            }
            else {
                char c[5] = "";
                keys[x] = c;        ///Instead of NULL the key will be an empty string.
            }
        }


	if (LOGGING == 1 || LOGGING == 2)
	    logger(log_file , message);			//Before it ends, if everything is okay, it will generate the log file, with the
							//description of what was made. I've placed it here because it's just before it returns '0'
							//(when everything went as planned). Monitoring client activity.
	return x;
    }
    else {
        errno = ERR_CONNECTION_FAIL;
        return -1;
    }

    errno = ERR_UNKNOWN;
    return -1;
}

