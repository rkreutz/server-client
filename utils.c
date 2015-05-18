/**
 * @file
 * @brief This file implements various utility functions that are
 * can be used by the storage server and client library.
 */

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "utils.h"

int sendall(const int sock, const char *buf, const size_t len)
{
	size_t tosend = len;
	while (tosend > 0) {
		ssize_t bytes = send(sock, buf, tosend, 0);
		if (bytes <= 0)
			break; // send() was not successful, so stop.
		tosend -= (size_t) bytes;
		buf += bytes;
	};

	return tosend == 0 ? 0 : -1;
}

/**
 * In order to avoid reading more than a line from the stream,
 * this function only reads one byte at a time.  This is very
 * inefficient, and you are free to optimize it or implement your
 * own function.
 */
int recvline(const int sock, char *buf, const size_t buflen)
{
	int status = 0; // Return status.
	size_t bufleft = buflen;

	while (bufleft > 1) {
		// Read one byte from scoket.
		ssize_t bytes = recv(sock, buf, 1, 0);
		if (bytes <= 0) {
			// recv() was not successful, so stop.
			status = -1;
			break;
		} else if (*buf == '\n') {
			// Found end of line, so stop.
			*buf = 0; // Replace end of line with a null terminator.
			status = 0;
			break;
		} else {
			// Keep going.
			bufleft -= 1;
			buf += 1;
		}
	}
	*buf = 0; // add null terminator in case it's not already there.

	return status;
}


/**
 * @brief Parse and process a line in the config file.
 */
int process_config_line(char *line, struct config_params *params, TableHeader* record_tables)
{
	// Ignore comments.
	if (line[0] == CONFIG_COMMENT_CHAR)
		return 0;

	// Extract config parameter name and value.
	char name[MAX_CONFIG_LINE_LEN];
	char value[MAX_CONFIG_LINE_LEN] , value2[MAX_CONFIG_LINE_LEN];
	int items = sscanf(line, "%s %s", name, value2);
	strcpy(value,value2);

	// Line wasn't as expected.
	if (items != 2)
		return 0;

	// Process this line.
	if (strcmp(name, "server_host") == 0) {
        if(strcmp(params->server_host , "") == 0)
            strncpy(params->server_host, value, sizeof params->server_host);
        else {
            printf("Duplicate parameters.\n");      ///Test to see if paramater was already provided
            return -1;
        }
	} else if (strcmp(name, "server_port") == 0) {
	    if(params->server_port == -1)
            params->server_port = atoi(value);
        else {
            printf("Duplicate parameters.\n");      ///Test to see if paramater was already provided
            return -1;
        }
	} else if (strcmp(name, "username") == 0) {
	    if(strcmp(params->username , "") == 0)
            strncpy(params->username, value, sizeof params->username);
        else {
            printf("Duplicate parameters.\n");      ///Test to see if paramater was already provided
            return -1;
        }
	} else if (strcmp(name, "password") == 0) {
	    if(strcmp(params->password , "") == 0)
            strncpy(params->password, value, sizeof params->password);
        else {
            printf("Duplicate parameters.\n");      ///Test to see if paramater was already provided
            return -1;
        }
	} else if (strcmp(name, "table") == 0) {
        Table* tmp;
        if (table_parser(value) == 0 ){
            tmp = record_tables->headTable;

            int z;
            char tester[2];
            z = sscanf(line , "%s %s %s",tester,tester,tester); ///Test if there is at least 3 strings inside line.

            if(z == 3) {    ///If there are at least 3 strings it means that we have columns.
                strtok(line , " ");
                strtok(NULL , " ");
                line = strtok(NULL , "");
            }
            else            ///If there are less than 3 it means there are no columns.
                sprintf(line,"0");

            if (tmp != NULL) {  ///If there is at least one table on the list
                if(record_tables->numTable >= MAX_TABLES){
                    printf("Max number of tables reached.\n");  ///Max number of tables reached.
                    return -1;
                }
                if (strcmp(tmp->nameTable , value) == 0){
                    printf("Duplicate parameters.\n");          ///Duplicate tables (first table).
                    return -1;
                }
                while(tmp->next_table != NULL){
                    tmp = tmp->next_table;
                    if (strcmp(tmp->nameTable , value) == 0){
                        printf("Duplicate parameters.\n");      ///Duplicate tables (everything after the first table).
                        return -1;
                    }
                }
                if (columns_parser(line) != 0 ) {       ///Parser for the columns.
                    printf("Problem with columns\n");
                    return -1;
                }
                Table* newTable = (Table*)malloc(sizeof(Table));///Creates a new table.
                newTable->headKey = NULL;
                newTable->next_table = NULL;
                newTable->numRec = 0;
                strcpy(newTable->nameTable , value);
                strcpy(newTable->columns , line);
                tmp->next_table = newTable;                     ///Link the new table to the last table.
                record_tables->numTable += 1;
            }
            else {  ///In case ther is no tables on the list, just create a new one and make it the head table.
                if (columns_parser(line) != 0 ) {
                    printf("Problem with columns\n");
                    return -1;
                }
                Table* newTable = (Table*)malloc(sizeof(Table));
                newTable->headKey = NULL;
                newTable->next_table = NULL;
                newTable->numRec = 0;
                strcpy(newTable->nameTable , value);
                strcpy(newTable->columns , line);
                record_tables->headTable = newTable;
                record_tables->numTable = 1;
            }
        }
        else {
            printf("Invalid parameter.\n");
            return -1;
        }
    } else if (strcmp(name , "concurrency") == 0) {
        if(params->concurrency == -1) {
            params->concurrency = atoi(value);
            if (params->concurrency > 3 && params->concurrency < 0)
                return -1;
        }
        else {
            printf("Duplicate parameters.\n");
            return -1;
        }
    }// else if (strcmp(name, "data_directory") == 0) {
	//	strncpy(params->data_directory, value, sizeof params->data_directory);
	//}
	else {
		// Ignore unknown config parameters.
	}

	return 0;
}


int read_config(const char *config_file, struct config_params *params, TableHeader* record_tables)
{
	int error_occurred = 0;
	int status;
	//Parameters are initialized so we can know if something has changed.
    params->server_port = -1;       ///Initialize all parameters to see if they will be specified on the config file.
    strcpy(params->server_host,"");
    strcpy(params->username,"");
    strcpy(params->password,"");
    params->concurrency = -1;

	// Open file for reading.
	FILE *file = fopen(config_file, "r");
	if (file == NULL)
		error_occurred = 1;

	// Process the config file.
	while (!error_occurred && !feof(file)) {
		// Read a line from the file.
		char line[MAX_CONFIG_LINE_LEN];
		char *l = fgets(line, sizeof line, file);

		// Process the line.
		if (l == line){
			status = process_config_line(line, params, record_tables);
			if (status != 0)
                error_occurred = 1;
		}
		else if (!feof(file))
			error_occurred = 1;
	}

    //If any parameter wasn't defined or it was defined as an empty space it will report an error.
	if( (strcmp(params->server_host , "") == 0 || params->server_port == -1 || params->server_port == 0 ||
        strcmp(params->username , "") == 0 || strcmp(params->password , "") == 0 || params->concurrency == -1)
        && error_occurred == 0) {
        printf("Missing or invalid parameter.\n");
        error_occurred = 1;
    }

	return error_occurred ? -1 : 0;
}

void logger(FILE *file, char *message)
{
	fprintf(file,"%s",message);
	fflush(file);
}

char *generate_encrypted_password(const char *passwd, const char *salt)
{
	if(salt != NULL)
		return crypt(passwd, salt);
	else
		return crypt(passwd, DEFAULT_CRYPT_SALT);
}

//Added

void generate_time_string_log(char* str) {
	time_t seconds;
	struct tm* tim;
	int x,len;

	time (&seconds);

	tim = gmtime(&seconds);

	len = sprintf( str , "%4d-%2d-%2d-%2d-%2d-%2d.log" , (tim->tm_year)+1900 , (tim->tm_mon)+1 , (tim->tm_mday) ,
		(tim->tm_hour)-5 , tim->tm_min , tim->tm_sec);

	//trading ' ' for '0'
	for( x = 0 ; x < len ; x++ ) {
		if (str[x] == ' ')
			str[x] = '0';
	}
}

void erase_string(char* str) {
	strcpy( str , "");
}

void free_memory(TableHeader* record_tables) {  ///This function is used to erase to whole list, when the server closes.
    Table *tmp, *nxt;
    Key *temp, *next;
    for(tmp = record_tables->headTable ; tmp != NULL ; tmp = nxt){
        for(temp = tmp->headKey; temp != NULL ; temp = next) {
            next = temp->next_key;
            free(temp);
        }
        nxt = tmp->next_table;
        free(tmp);
    }
    record_tables->headTable = NULL;
    record_tables->numTable = 0;
}

int table_parser(char *name){       ///Parser for the table name
    int x;
    int y;
    if (strlen(name) > MAX_TABLE_LEN){  ///Can't be more than MAX_TABLE_LEN
        return -1;
    }
    for (y = 0 ; y < strlen(name) ; y++) {  ///Just numbers and letters are accepted.
        if ( !( ( name[y] >= 48 && name[y] <= 57 ) || ( name[y] >= 65 && name[y] <= 90 ) ||
                ( name[y] >= 97 && name[y] <= 122 ) ) )
            return -1;
    }
    return 0;
}

int key_parser(char *name) {        ///Parser for the key name, basically the same of the table parser.
    if (strlen(name) > MAX_KEY_LEN)
        return -1;
    int x;
    for (x = 0 ; x < strlen(name) ; x++) {
        if ( !( ( name[x] >= 48 && name[x] <= 57 ) || ( name[x] >= 65 && name[x] <= 90 ) ||
                ( name[x] >= 97 && name[x] <= 122 ) ) )
            return -1;
    }
    return 0;
}
int value_parser(char *name) {  ///Value parser, same of the other parser, but it can have ' ' (spaces) inside.
    if (strlen(name) > MAX_VALUE_LEN)
        return -1;
    int x;
    for (x = 0 ; x < strlen(name) ; x++) {
        if ( !( ( name[x] >= 48 && name[x] <= 57 ) || ( name[x] >= 65 && name[x] <= 90 ) ||
                ( name[x] >= 97 && name[x] <= 122 ) || (name[x] == ' ') || ((name[x] == ',') && name[x+1] != '\0') ||
                  name[x] == '-' || name[x] == '+'  ) )
            return -1;
    }
    return 0;
}

Table* find_table (char *name , TableHeader* header){   ///Function used to find a table
    Table *tmp;
    tmp = header->headTable;
    while(tmp != NULL){     ///While the pointer isn't null it goes searching for the table with the specified name.
        if (strcmp(name , tmp->nameTable) == 0)
            break;
        else
            tmp = tmp->next_table;
    }
    return tmp;     ///return the address of the table found (if not it will be null).
}

Key* find_key (char *name , Table* table_) {    ///finds a key inside a table, same thing basically of the find table.
    Key *tmp;
    tmp = table_->headKey;
    while(tmp != NULL){
        if (strcmp(name , tmp->nameKey) == 0)
            break;
        else
            tmp = tmp->next_key;
    }
    return tmp;
}

void add_key (char *name , struct storage_record value , Table* table_) {   ///function used to add keys to tables.
    Key *tmp;
    if (table_->headKey == NULL) {      ///If there is no key on the table.
        tmp = (Key*)malloc(sizeof(Key));///Creates a key.
        strcpy(tmp->nameKey , name);
        strcpy(tmp->record.value , value.value);
        memset(tmp->record.metadata , 1 , sizeof(tmp->record.metadata));
        tmp->next_key = NULL;
        table_->headKey = tmp;          ///Links the key to the head of the key list.
    }
    else {
        for (tmp = table_->headKey ; tmp->next_key != NULL ; tmp = tmp->next_key);  ///goes to the last key of the list.
        if (tmp != NULL) {
            tmp->next_key = (Key*)malloc(sizeof(Key));  ///Creates a new key and link it to the last key of the table.
            strcpy(tmp->next_key->nameKey , name);
            strcpy(tmp->next_key->record.value , value.value);
            memset(tmp->next_key->record.metadata , 1 , sizeof(tmp->next_key->record.metadata));
            tmp->next_key->next_key = NULL;
        }
    }
    table_->numRec += 1;
    return;
}

void delete_key (Key* key_ , Table* table_) {
    Key *tmp;
    if(table_->headKey == key_){    ///If the head key is the key we want to delete.
        table_->headKey = key_->next_key;   ///Links the next key as the head key.
        free(key_); ///Deletes the desired key.
    }
    else{
        for (tmp = table_->headKey ; tmp ->next_key != key_ ; tmp = tmp->next_key); ///finds the last key before the one desired.
        tmp ->next_key = key_->next_key;    ///link the key before with the key after the one we want to delete.
        free(key_);     ///deletes the key.
    }
    table_->numRec -= 1;
}

int columns_parser (char *line) {
    char *columns[MAX_COLUMNS_PER_TABLE + 1];	///Array of strings. It's big enough to store one more than it should.
    int numCol , z , y , x = 0;
    if (strcmp(line,"0") == 0){         ///If line = "0" it means there are no columns so return error.
        return -1;
    }

    char line2[MAX_CONFIG_LINE_LEN];    ///Copy of line
    strcpy(line2 , line);

    columns[0] = strtok(line2 , ",");    ///First string
    while (columns[x] != NULL && x <= MAX_COLUMNS_PER_TABLE) {///If last string wasn't null it will add '1' to the number
        x++;                                                  ///of columns and then it will go to the next string.
        columns[x] = strtok(NULL , ",");
    }

    numCol = x;
    if (numCol > MAX_COLUMNS_PER_TABLE)
        return -1;

    char *type[numCol*2];   ///Array string to hold type of each column.

    for(x = 0 ; x < numCol ; x++) {
        strtok(columns[x] , ":");  ///Separates column name from column type.
        type[x] = strtok(NULL , "\n");
        erase_space_ends(type[x]);
        erase_space_ends(columns[x]);

        if (strlen(columns[x]) > MAX_COLNAME_LEN)
            return -1;

        for(y = 0 ; columns[x][y] != '\0' ; y++) {  ///While character not end of string.
            if (!isalnum(columns[x][y]))        ///If character is not alphanumeric.
                return -1;
        }

        for(y = 0 ; type[x][y] != '\0' ; y++) {
            if (!isalnum(type[x][y]) && type[x][y] != '[' && type[x][y] != ']'){///If character not alphanumeric or "[]".
                return -1;
            }
        }

        if (strncmp(type[x],"int",3) == 0) {   ///If the type is "int".
            strcpy(type[x] , "0");
        }
        else if(strncmp(type[x],"float",5) == 0) {  ///If the type is "float".
            printf("Float still not supported.\n");
            return -1;
        }
        else if(strncmp(type[x],"char[",5) == 0){    ///If the type is "char".
            strtok(type[x] , "[");
            type[x] = strtok(NULL , "]");
            if(type[x] == NULL) ///If there is no second braket.
                return -1;

            y = strlen(type[x]);

            if (y < 1)      ///If brakets are empty.
                return -1;

            for(z = 0 ; z < y ; z++) {
                if( !isdigit(type[x][z]) )  ///If there is a non numerical value.
                    return -1;
            }

            if(atoi(type[x]) > MAX_STRTYPE_SIZE)
                return -1;
        }
        else{
            return -1;
        }

    }
    ///Compare all name of columns to see if there is any duplicated column.
    for (x = 0 ; x < numCol ; x++) {
        for(y = x+1 ; y < numCol ; y++){
            if(strcmp(columns[x],columns[y]) == 0)
                return -1;
        }
    }

    ///Returning all information parsed to string 'line'.
    erase_string(line);
    sprintf(line,"%d ",numCol);
    for(x = 0 ; x < numCol ; x++){
        strcat(line , columns[x]);
        strcat(line , " ");
        strcat(line , type[x]);
        if (x != numCol - 1)
            strcat(line , ",");
    }

    return 0;
}

void erase_space_ends (char *str){
    int x;
    if(str != NULL) {               ///If not a null string continue.
        while(isblank(str[0]) != 0){ ///If first character is a space, pointer to the first character will move to the next
            for(x = 0 ; x < strlen(str) ; x++){ ///character, and this process goes on until there is no white space on the begining.
                str[x] = str[x+1];
            }
        }
        for(x = 0 ; x < strlen(str) ; x++){
            if(isspace(str[x]) != 0 && str[x+1] == '\0') {
                str[x] = '\0';  ///If the last character is a space, remove it
                x = 0;          ///and start the loop again to check for other spaces in the end.
            }
        }
    }
}

char *query_parsing(Table *temp , char *predicates_line){
    char *column[2*MAX_COLUMNS_PER_TABLE];  ///table name from predicate
    char *value[2*MAX_COLUMNS_PER_TABLE];   ///table value from predicate
    char opperand[2*MAX_COLUMNS_PER_TABLE]; ///opperand used on predicate
    char keys[MAX_VALUE_LEN] = "";          ///keys to be returned
    int x , y , z;
    int numPred = 0;                        ///number of predicates
    while (predicates_line != NULL) {
        erase_space_ends(predicates_line);  ///take out spaces from both ends
        x = strcspn(predicates_line , "<>=");       ///Position of the predicate symbol
        if(x != 0) {
            if(x != strlen(predicates_line)) {
                opperand[numPred] = predicates_line[x];             ///storing predicate symbol.
                column[numPred] = strtok(predicates_line , "<>=");  ///storing column name from predicate.
                erase_space_ends(column[numPred]);
                value[numPred] = strtok(NULL , ",");                ///storing column value from predicate.
                erase_space_ends(value[numPred]);
                if(column[numPred] != NULL && (value[numPred] == NULL || strcmp(value[numPred],"") == 0) ) { ///Unmatching predicates
                    x = -1;
                    predicates_line = NULL;
                }
                else {
                    numPred++;
                    predicates_line = strtok(NULL,"");  ///Copy of the current line that wasn't parsed.
                }
            }
            else {  ///If there is no opperand.
                x = -1;
                predicates_line = NULL;
            }
        }
        else if(strlen(predicates_line) == 0) {                 ///empty line, print all.
            opperand[numPred] = 'a';
            predicates_line = strtok(NULL,"");  ///Copy of the current line that wasn't parsed.
        }
        else {      ///If first element of predicates_line is "<>=" it's an invalid parameter.
            x = -1;
            predicates_line = NULL;
        }
    }
    if (x != -1){   ///Will return error if predicates are wrong in any possible way that doesn't relate to the table.
        ///Check to see if predicates are valid
        char colTable[MAX_VALUE_LEN];                     ///String being used for operations containing columns from server.
        int found = -1;                     ///Check for valid column specified in predicate.
        int valid = 0;                      ///Check for valid predicate.
        if(opperand[0] == 'a'){             ///No predicates.
            found = 0;
        }
        else {
            for (x = 0 ; x < numPred ; x++) {   ///Loop to check all predicates.
                strcpy(colTable , temp->columns);      ///Each time it changes the predicate it has to load the initial column string.
                int numCols = atoi(strtok(colTable," "));
                found = -1;
                if(opperand[x] != 'a') {        ///If any other opperand was set to 'a' there is a problem.
                    for(y = 0 ; y < numCols ; y++) {///Loop to check all columns.
                        if(strcmp(column[x] , strtok(NULL , " ")) == 0){    ///If predicate column is found.
                            found = 0;
                            y = numCols;
                            char *colType = strtok(NULL , ",");
                            if(strcmp(colType , "0") == 0) {     ///Case is a integer.
                                for(z = 0 ; z < (strlen(value[x])) ; z++) { ///Loop to see if value is a int.
                                    if (z != 0) {       ///If it's not the first character
                                        if(!isdigit(value[x][z])) {             ///If a single character is not a digit.
                                            valid = -1;
                                            z = strlen(value[x]);
                                        }
                                    }
                                    else{               ///If it's the first character
                                        if(!isdigit(value[x][z]) && value[x][z] != '+' && value[x][z] != '-') {///If a single character is not a digit.
                                            valid = -1;
                                            z = strlen(value[x]);
                                        }
                                    }
                                }
                            }
                            else {                                          ///Case is a string.
                                if( value_parser(value[x]) != 0 ||          ///If value is not valid.
                                    strlen(value[x]) > atoi(colType) - 1 || ///If value is larger than it could be
                                    opperand[x] != '=')                     ///If opperand is not '='
                                        valid = -1;
                            }
                        }
                        else {
                            strtok(NULL , ",");
                        }
                    }
                }
                else
                    valid = -1;

                if(found == -1 || valid == -1)
                    x = numPred;
            }
        }

        ///Everything is valid so let's start the query.
        if(found == 0 && valid == 0){
            Key *key = temp->headKey;
            if(opperand[0] == 'a'){ ///Send all keys.
                while(key != NULL){
                    strcat(keys , key->nameKey);
                    strcat(keys , " ");
                    key = key->next_key;
                }
            }
            else {                  ///Regular query.
                while(key != NULL){
                    char rec[MAX_VALUE_LEN];
                    char *str;
                    int ff = 0;     ///Number of predicates fulfilled.
                    for(x = 0 ; x < numPred ; x++){
                        strcpy(rec , key->record.value);
                        str = strtok(rec , " ,");
                        while(strcmp(str , column[x]) != 0) {
                            str = strtok(NULL , " ,");
                        }

                        if(opperand[x] == '='){
                            str = strtok(NULL , ",");
                            erase_space_ends(str);
                            if(strcmp(str,value[x]) == 0)
                                ff++;
                        }
                        else if(opperand[x] == '<'){
                            str = strtok(NULL , ",");
                            erase_space_ends(str);
                            if(atoi(str) < atoi(value[x]))
                                ff++;
                        }
                        else if(opperand[x] == '>'){
                        str = strtok(NULL , ",");
                        erase_space_ends(str);
                        if(atoi(str) > atoi(value[x]))
                            ff++;
                        }
                    }
                    if(ff == numPred){  ///If all predicates fullfilled, send the key.
                        strcat(keys , key->nameKey);
                        strcat(keys , " ");
                    }
                    key = key->next_key;
                }
            }
            return keys;
        }
    }
    return ("error 1");     ///Invalid parameter.
}


