/**
 * @file
 * @brief This file declares various utility functions that are
 * can be used by the storage server and client library.
 */

#ifndef	UTILS_H
#define UTILS_H

#include <stdio.h>
#include "storage.h"

/**
 * @brief Any lines in the config file that start with this character
 * are treated as comments.
 */
static const char CONFIG_COMMENT_CHAR = '#';

/**
 * @brief The max length in bytes of a command from the client to the server.
 */
#define MAX_CMD_LEN (1024 * 8)

/**
 * @brief A macro to log some information.
 *
 * Use it like this:  LOG(("Hello %s", "world\n"))
 *
 * Don't forget the double parentheses, or you'll get weird errors!
 */
#define LOG(x)  {printf x; fflush(stdout);}
#define LOGGING 0
FILE *log_file;

/**
 * @brief A macro to output debug information.
 *
 * It is only enabled in debug builds.
 */
#ifdef NDEBUG
#define DBG(x)  {}
#else
#define DBG(x)  {printf x; fflush(stdout);}
#endif

/**
 * @brief A struct to store config parameters.
 */
struct config_params {
	/// The hostname of the server.
	char server_host[MAX_HOST_LEN];

	/// The listening port of the server.
	int server_port;

	/// The storage server's username
	char username[MAX_USERNAME_LEN];

	/// The storage server's encrypted password
	char password[MAX_ENC_PASSWORD_LEN];

	/// The status of the authentication on the server.
	int status_auth;

	/// Status for concurrency.
	int concurrency;

	/// The directory where tables are stored.
	char data_directory[MAX_PATH_LEN];
};

//Structure for list of keys with values.
typedef struct key{
    char nameKey[MAX_KEY_LEN];
    struct storage_record record;
    struct key* next_key;
} Key;
//Structure for list of tables with the head of the key list and number of records.
typedef struct table{
    char nameTable[MAX_TABLE_LEN];
    char columns[MAX_VALUE_LEN];  ///"a bbb ccc, ddd eee , ..." a = number of columns; 'bbb' and 'ddd' = name of column;
    int numRec; ///number of records    ///'ccc' and 'eee' = size and type ----> if 'ccc' = 0 it's an integer; any other number
    Key* headKey;                       ///is the size of the string e.g. 'ccc' = 50 is a char[50].
    struct table* next_table;
} Table;
//Head of the table, with number of tables.
typedef struct table_header{
    int numTable;   ///number of tables.
    Table* headTable;
} TableHeader;

typedef struct argument_for_thread {
    struct config_params parameters_server;
    int clientsock;
    char client_addr[40];
    int client_port;
} ArgThread;
/**
 * @brief Exit the program because a fatal error occured.
 *
 * @param msg The error message to print.
 * @param code The program exit return value.
 */
static inline void die(char *msg, int code)
{
	printf("%s\n", msg);
	exit(code);
}

/**
 * @brief Keep sending the contents of the buffer until complete.
 * @return Return 0 on success, -1 otherwise.
 *
 * The parameters mimic the send() function.
 */
int sendall(const int sock, const char *buf, const size_t len);

/**
 * @brief Receive an entire line from a socket.
 * @return Return 0 on success, -1 otherwise.
 */
int recvline(const int sock, char *buf, const size_t buflen);

/**
 * @brief Read and load configuration parameters.
 *
 * @param config_file The name of the configuration file.
 * @param params The structure where config parameters are loaded.
 * @return Return 0 on success, -1 otherwise.
 */
int read_config(const char *config_file, struct config_params *params, TableHeader* record_tables);

/**
 * @brief Generates a log message.
 *
 * @param file The output stream
 * @param message Message to log.
 */
void logger(FILE *file, char *message);

/**
 * @brief Default two character salt used for password encryption.
 */
#define DEFAULT_CRYPT_SALT "xx"

/**
 * @brief Generates an encrypted password string using salt CRYPT_SALT.
 *
 * @param passwd Password before encryption.
 * @param salt Salt used to encrypt the password. If NULL default value
 * DEFAULT_CRYPT_SALT is used.
 * @return Returns encrypted password.
 */
char *generate_encrypted_password(const char *passwd, const char *salt);

//Added

void generate_time_string_log(char* str);
void erase_string(char* str);
void free_memory(TableHeader* record_tables);
int table_parser(char *name);
int key_parser(char *name);
int value_parser(char *name);
Table* find_table (char *name , TableHeader* header);
Key* find_key (char *name , Table* table_);
void add_key (char *name , struct storage_record value , Table* table_);
void delete_key (Key* key_ , Table* table_);
int columns_parser (char *line);
void erase_space_ends (char *str);
char *query_parsing(Table *temp , char *predicates_line);

#endif
