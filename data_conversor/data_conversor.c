#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void rename_city(char *city) {
    city = strtok(city , "(");
    int x,y;
    for (x = 0 ; x < strlen(city) ; x++) {
        if ( !( ( city[x] >= 48 && city[x] <= 57 ) || ( city[x] >= 65 && city[x] <= 90 ) ||
                ( city[x] >= 97 && city[x] <= 122 ) ) ) {
            for (y = x ; y <= strlen(city) ; y++)
                city[y] = city[y+1];
            x--;
        }
    }
}

int main(int argc , char *argv[]){
    if (argc != 2) {
        printf("Usage %s <data file>\n" , argv[0]);
        return EXIT_FAILURE;
    }
    FILE *pfile;
    pfile = fopen( argv[1] , "r");
    if (pfile == NULL) {
        printf("File doesn't exist.\n");
        return EXIT_FAILURE;
    }
    char *city , *pop , line[500];
    FILE* pfile2 = fopen("census" , "w");
    char c = 50;
    while (c!=EOF){
        fgets(line , 500 , pfile);
        strtok(line , "\"");
        city = strtok(NULL,"\"");
        strtok(NULL,","); strtok(NULL,",");
        pop = strtok(NULL,",");
        rename_city(city);
        c = fgetc(pfile);
        fprintf(pfile2 , "%s %s\n" , city , pop);
    }
    fclose(pfile);
    fclose(pfile2);
    return 0;
}
