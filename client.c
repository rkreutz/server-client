/**
 * @file
 * @brief This file implements a "very" simple sample client.
 *
 * The client connects to the server, running at SERVERHOST:SERVERPORT
 * and performs a number of storage_* operations. If there are errors,
 * the client exists.
 */

#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "storage.h"
#include "utils.h"


/**
 * @brief Start a client to interact with the storage server.
 *
 * If connect is successful, the client performs a storage_set/get() on
 * TABLE and KEY and outputs the results on stdout. Finally, it exists
 * after disconnecting from the server.
 */
int main(int argc, char *argv[]) {
    int select = 0 , x =0 ;
    double elapsedTime = 0;
    char SERVERHOST[MAX_HOST_LEN] , SERVERUSERNAME[MAX_USERNAME_LEN] , SERVERPASSWORD[MAX_ENC_PASSWORD_LEN];
    char TABLE[MAX_TABLE_LEN] , KEY[MAX_KEY_LEN] , val[MAX_VALUE_LEN] , predicates[MAX_CMD_LEN];
    int SERVERPORT , status;
    void *conn;
    struct storage_record r;
    char time_log[25] , name[40] ;
    memset((r.metadata) , 0 , sizeof(r.metadata));
    generate_time_string_log(time_log);
    sprintf(name , "Client-%s", time_log );
    if (LOGGING == 2) {
        log_file = fopen( name , "w" );
        if (log_file == NULL)
            printf("CLIENT: An error occured genereting the log file\n");
    }
    else if (LOGGING == 1)
        log_file = stdout;

    while(select != 8){
        struct timeval t1, t2;
        printf("1) Connect\n2) Authenticate\n3) Set\n4) Delete\n5) Get\n6) Query\n7) Disconnect\n8) Exit\n\n> ");
        scanf("%d",&select);

        if(select == 1){
        // Connect to server
            printf("Hostname: ");
            scanf("%s",SERVERHOST);
            printf("Port: ");
            scanf("%d",&SERVERPORT);

            gettimeofday(&t1, NULL);
            conn = storage_connect(SERVERHOST, SERVERPORT);
            if(!conn) {
                printf("Cannot connect to server @ %s:%d. Error code: %d.\n\n\n",
                        SERVERHOST, SERVERPORT, errno);
            }
            else
                printf("Connected to server.\n");
            gettimeofday(&t2, NULL);
            elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;
            elapsedTime += (t2.tv_sec - t1.tv_sec);
            //printf("%.5f\n" , elapsedTime);       ///Used for showing the performance.
        }

        else if(select == 2) {
        // Authenticate the client.
            printf("Username: ");
            scanf("%s",SERVERUSERNAME);
            printf("Password: ");
            scanf("%s",SERVERPASSWORD);
            gettimeofday(&t1, NULL);
            status = storage_auth(SERVERUSERNAME, SERVERPASSWORD, conn);
            if(status != 0) {
                printf("storage_auth failed with username '%s' and password '%s'. " \
                        "Error code: %d.\nDisconnected from server.\n\n\n", SERVERUSERNAME, SERVERPASSWORD, errno);
                storage_disconnect(conn);
            }
            else
                printf("storage_auth: successful.\n");
            gettimeofday(&t2, NULL);
            elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;
            elapsedTime += (t2.tv_sec - t1.tv_sec);
            //printf("%.5f\n" , elapsedTime);
        }

        else if(select == 3) {     // Issue storage_set
            printf("Table: ");
            scanf("%s",TABLE);
            printf("Key: ");
            scanf("%s",KEY);
            printf("Value: ");
	    gets(val);
            while(strcmp(val , "") == 0){
                fflush(stdin);
                gets(val);
            }
            gettimeofday(&t1, NULL);
            strncpy(r.value, val, sizeof r.value);
            status = storage_set(TABLE, KEY, &r, conn);
            if(status != 0) {
                printf("storage_set failed. Error code: %d.\n\n\n", errno);
                //return status;
            }
            else
                printf("storage_set: successful.\n");
            gettimeofday(&t2, NULL);
            elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;
            elapsedTime += (t2.tv_sec - t1.tv_sec);
            //printf("%.5f\n" , elapsedTime);
        }

        else if(select == 5) {     // Issue storage_get
            printf("Table: ");
            scanf("%s",TABLE);
            printf("Key: ");
            scanf("%s",KEY);
            gettimeofday(&t1, NULL);
            status = storage_get(TABLE, KEY, &r, conn);
            if(status != 0) {
                printf("storage_get failed. Error code: %d.\n\n\n", errno);
                //return status;
            }
            else
                printf("storage_get: the value returned for key '%s' is '%s'.\n", KEY, r.value);
            gettimeofday(&t2, NULL);
            elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;
            elapsedTime += (t2.tv_sec - t1.tv_sec);
            //printf("%.5f\n" , elapsedTime);
        }

        else if (select == 7) { // Disconnect from server
            gettimeofday(&t1, NULL);
            status = storage_disconnect(conn);
            if(status != 0) {
                printf("storage_disconnect failed. Error code: %d.\n\n\n", errno);
                //return status;
            }
            gettimeofday(&t2, NULL);
            elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;
            elapsedTime += (t2.tv_sec - t1.tv_sec);
            //printf("%.5f\n" , elapsedTime);
            printf("Disconnected from server.\n");
        }

        else if (select == 6) {
            printf("Table: ");
            scanf("%s" , TABLE);
            int max_keys;
            printf("Max number of keys: ");
            scanf("%d",&max_keys);
            char *keys_returned[max_keys];
            printf("Predicates: ");
            gets(predicates);
            while(strcmp(predicates , "") == 0){
                fflush(stdin);
                gets(predicates);
            }

            status = storage_query(TABLE,predicates,keys_returned,max_keys,conn);
            if(status == -1) {
                printf("storage_query failed. Error code: %d \n\n\n",errno);
            }
            else {
                int x;
                printf("Total of %d keys retrieved.\n",status);
                if(max_keys <= status){
                    for(x = 0 ; x < max_keys ; x++)
                        storage_get(TABLE,keys_returned[x],&r,conn);
                }
                else{
                    for(x = 0 ; x < status ; x++)
                        storage_get(TABLE,keys_returned[x],&r,conn);
                }
            }
        }
	else if (select == 4) {
	    printf("Table: ");
	    scanf("%s" , TABLE);
	    printf("Key: ");
	    scanf("%s" , KEY);
	    gettimeofday(&t1, NULL);
	    status = storage_set(TABLE, KEY, NULL, conn);
            if(status != 0) {
                printf("storage_set failed. Error code: %d.\n\n\n", errno);
                //return status;
            }
            else
                printf("storage_set: successful.\n");
            gettimeofday(&t2, NULL);
            elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;
            elapsedTime += (t2.tv_sec - t1.tv_sec);
            //printf("%.5f\n" , elapsedTime);
	}

    }
    //printf("%.5f\n" , elapsedTime);
    if (LOGGING == 2)
    fclose(log_file);
     // Exit
    return 0;
}
