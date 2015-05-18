#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <netdb.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "../../storage.h"
#include "../../utils.h"

#define HOST "localhost"
#define PORT 1111
#define USERNAME "admin"
#define PASSWORD "admin"
#define TABLE "table1"
#define KEY1 "key1"
#define KEY2 "key2"
#define KEY3 "key3"
#define KEY4 "key4"
#define KEY5 "key5"
#define VALUE1 "col1 1 , col2 abc"
#define VALUE2 "col1 2 , col2 def"
#define VALUE3 "col1 3 , col2 ghi"
#define VALUE4 "col1 4 , col2 jkl"
#define VALUE5 "col1 5 , col2 mno"
#define initial_time 1366060890


void *var;
struct timeval t1 , t2;
double total;
struct storage_record record;
int abortRatio = 0;


void process1(void *sock , int x) {
    int status;
    char *keys[50];
    if( !storage_auth(USERNAME , PASSWORD , sock)) printf("%d authenticated.\n", x);
    while(total < initial_time){
        gettimeofday(&t1 , NULL);
        total = t1.tv_sec;
    }
    gettimeofday(&t1 , NULL);
    memset(record.metadata , 0 , sizeof(record.metadata));
    strcpy(record.value , VALUE1);
    storage_set(TABLE , KEY1 , &record , sock);
    storage_set(TABLE , KEY2 , &record , sock);
    storage_set(TABLE , KEY3 , &record , sock);
    storage_set(TABLE , KEY4 , &record , sock);
    storage_set(TABLE , KEY5 , &record , sock);

    //First transaction
    storage_get(TABLE , KEY1 , &record , sock);
    strcpy(record.value , VALUE2);
    status = storage_set(TABLE , KEY1 , &record , sock);
    if (status != 0 && errno == 8)
        abortRatio++;

    storage_get(TABLE , KEY3 , &record , sock);
    storage_set(TABLE , KEY5 , NULL , sock);
    memset(record.metadata , 0 , sizeof(record.metadata));
    strcpy(record.value , VALUE5);
    storage_set(TABLE , KEY5 , &record , sock);

    //Second transaction
    storage_get(TABLE , KEY2 , &record , sock);
    strcpy(record.value , VALUE5);
    status = storage_set(TABLE , KEY2 , &record , sock);
    if (status != 0 && errno == 8)
        abortRatio++;

    storage_query(TABLE , " " , keys , 5 , sock);

    //Third transaction
    storage_get(TABLE , KEY4 , &record , sock);
    strcpy(record.value , VALUE1);
    status = storage_set(TABLE , KEY4 , &record , sock);
    if (status != 0 && errno == 8)
        abortRatio++;

    memset(record.metadata , 0 , sizeof(record.metadata));
    strcpy(record.value , VALUE1);
    storage_set(TABLE , KEY1 , &record , sock);

    //Fourth transaction
    storage_get(TABLE , KEY3 , &record , sock);
    strcpy(record.value , VALUE5);
    status = storage_set(TABLE , KEY3 , &record , sock);
    if (status != 0 && errno == 8)
        abortRatio++;

    //Fifth transaction
    storage_get(TABLE , KEY5 , &record , sock);
    strcpy(record.value , VALUE3);
    status = storage_set(TABLE , KEY5 , &record , sock);
    if (status != 0 && errno == 8)
        abortRatio++;

    //Sixth transaction
    storage_get(TABLE , KEY2 , &record , sock);
    strcpy(record.value , VALUE3);
    status = storage_set(TABLE , KEY2 , &record , sock);
    if (status != 0 && errno == 8)
        abortRatio++;

    //Seventh transaction
    storage_get(TABLE , KEY1 , &record , sock);
    strcpy(record.value , VALUE1);
    status = storage_set(TABLE , KEY1 , &record , sock);
    if (status != 0 && errno == 8)
        abortRatio++;

    //Eight transaction
    storage_get(TABLE , KEY5 , &record , sock);
    strcpy(record.value , VALUE1);
    status = storage_set(TABLE , KEY5 , &record , sock);
    if (status != 0 && errno == 8)
        abortRatio++;

    //Nineth transaction
    storage_get(TABLE , KEY2 , &record , sock);
    strcpy(record.value , VALUE4);
    status = storage_set(TABLE , KEY2 , &record , sock);
    if (status != 0 && errno == 8)
        abortRatio++;

    //Tenth transaction
    storage_get(TABLE , KEY4 , &record , sock);
    strcpy(record.value , VALUE2);
    status = storage_set(TABLE , KEY4 , &record , sock);
    if (status != 0 && errno == 8)
        abortRatio++;

    storage_query(TABLE , " " , keys , 3 , sock);

    gettimeofday(&t2 , NULL);
    total = t2.tv_sec - t1.tv_sec;
    total += ((t2.tv_usec - t1.tv_usec)/1000000.0);
    storage_disconnect(sock);

    float AR = abortRatio/10.0;
    FILE *pfile;
    char file[30];
    sleep(2*x);
    printf("%d\n",x);
    sprintf(file , "data%d" , x);
    pfile = fopen(file , "w");
    fprintf(pfile , "Total time of execution: %f     \n\n" , total);
    fprintf(pfile , "Transaction abort rate: %.3f\n\n" , AR);
    fclose(pfile);
}

int main()
{
    var = mmap(NULL , sizeof(int) , PROT_READ|PROT_WRITE , MAP_SHARED , -1 , 0);
    pid_t child;
    int x;
    gettimeofday(&t1 , NULL);
    total = t1.tv_sec;
    printf("%.f" , initial_time - total);


    for(x = 0 ; x < 10 ; x++) {
        if((child = fork()) == 0) {
            void *sock = storage_connect(HOST , PORT);
            var = sock;
            if(!var){
                printf("Error connecting %d\n",x);
                exit(-1);
            }

            process1(sock , x);

            exit(0);
        }
        sleep(2);
    }
    while(total < initial_time){
        gettimeofday(&t1 , NULL);
        total = t1.tv_sec;
    }

    sleep(30);

    return 0;
}
