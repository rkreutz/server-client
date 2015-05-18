#include <stdio.h>
#include <stdlib.h>
#include "structure.h"

int main()
{
    char name[20];
    TableHeader tables;
    Table* tmp;
    tables.head = NULL ;
    tables.numTable = 0;
    while(1){
        printf("Type a name for your table: ");
        scanf("%s",name);
        tmp = (Table*)malloc(sizeof(Table));

        if(tables.head == NULL)
            tables.head = tmp;
        else {
            Table* tmp2;
            tmp2 = tables.head;
            if (strcmp(tmp2 ->name , name) == 0)
                printf("Entry already exists.\n");
            while(tmp2->next != NULL){
                if (strcmp(tmp2 ->name , name) == 0)
                    printf("Entry already exists.\n");
                tmp2 = tmp2 ->next;
            }
            tmp2 ->next = tmp;
        }
        tmp->head = NULL;
        tmp->next = NULL;
        tmp->numKey = 0;
        tables.numTable += 1;
        strcpy(tmp->name , name);
    }





    return 0;
}
