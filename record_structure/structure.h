#ifndef STRUCTURE_H_INCLUDED
#define STRUCTURE_H_INCLUDED

typedef struct key{
    char name[20];
    char value[20];
    struct key* next_key;
} Key;

typedef struct table{
    char name[20];
    int numKey;
    Key* headKey;
    struct table* next_table;
} Table;

typedef struct table_header{
    int numTable;
    Table* headTable;
} TableHeader;

#endif // STRUCTURE_H_INCLUDED
