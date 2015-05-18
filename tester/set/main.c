#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <check.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "storage.h"

#define HOSTNAME "localhost"
#define PORT 5001
#define ADMIN "admin"
#define DEC_PASSWORD "dog4sale"
#define TABLE1 "table1"
#define TABLE2 "table2"
#define INV_TABLE "bad table"
#define KEY1 "key1"
#define KEY2 "key2"
#define INV_KEY "bad key"
#define VALUE1 "col1 string"
#define VALUE2 "col1 string , col2 10"
#define INV_VALUE "col1 string , col2 string"


int main(){
    void *conn = storage_connect(HOSTNAME , PORT);
    if(conn == NULL) {
        printf("Error starting the server, please make sure the socket is not being used.\n");
        printf("Notice that the parameters inside the *.conf file should be as follows:\n");
        printf("Hostname: %s\n",HOSTNAME);
        printf("Port: %d\n",PORT);
        return -1;
    }
    int status = storage_auth(ADMIN , DEC_PASSWORD , conn);
    if (status != 0) {
        printf("Error authenticating the server, make sure that the parameters inside the *.conf file are as follows:\n");
        printf("Username: %s\n",ADMIN);
        printf("Decrypted Password: %s\n",DEC_PASSWORD);
        return -1;
    }

    printf("\n\nTest 1: ");
    struct storage_record record;                                     //test case 1 for set
    strcpy(record.value,VALUE1);
    status = storage_set(TABLE1, KEY1, &record, conn);
    if (status != 0) {
        printf("The test failed to add a key.");

    }
    else {
        status = storage_get(TABLE1, KEY1, &record, conn);
        if (status != 0) {
            printf("The test failed to get the value.");
        }
        else if ((strcmp(record.value, VALUE1) == 0)) {
            printf("Test is ok.");
        }
        else {
            printf("The test failed, not storing correctly.");
        }
    }


    printf("\n\nTest 2: ");
    strcpy(record.value,VALUE2);                                      //test case 2 for set
    status = storage_set(TABLE2, KEY2, &record, conn);
    if (status != 0) {
        printf("The test failed to add a key.");
    }
    else {
        status = storage_get(TABLE2, KEY2, &record, conn);
        if (status != 0) {
            printf("The test failed to get the value.");
        }
        else if ((strcmp(record.value, VALUE2) == 0)) {
            printf("Test is ok.");
        }
        else {
            printf("The test failed, not storing correctly.");
        }
    }

    printf("\n\nTest 3: ");
    strcpy(record.value,INV_VALUE);                                   //test case 3 for set
    status = storage_set(TABLE2, KEY2, &record, conn);
    if (status == 0) {
       printf("The test failed, should have occured an error.");
       return -1;
    }
    else if(errno != 1){
    	printf("The test failed, errno not being set properly");
    }
    else {
        printf("Test is ok.");
    }



    printf("\n\nTest 4: ");
    strcpy(record.value,VALUE1);                                      //test case 4 for set
    status = storage_set(INV_TABLE, KEY1, &record, conn);
    if (status == 0) {
       printf("The test failed, should have occured an error.");
       return -1;
    }
    else if(errno != 1){
    	printf("The test failed, errno not being set properly");
    }
    else {
        printf("Test is ok.");
    }



    printf("\n\nTest 5: ");
    strcpy(record.value,VALUE2);                                      //test case 5 for set
    status = storage_set(TABLE2, INV_KEY, &record, conn);
    status = storage_set(INV_TABLE, KEY1, &record, conn);
    if (status == 0) {
       printf("The test failed, should have occured an error.");
       return -1;
    }
    else if(errno != 1){
    	printf("The test failed, errno not being set properly");
    }
    else {
        printf("Test is ok.");
    }
    printf("\n\n");

    return 0;
}
