#ifndef symtable.h
#define symtable .h

typedef struct
{

} TTable;

typedef struct
{
    char *key;
} TKey;

typedef struct
{

} TData;

TTable Init();
TTable Insert(TTable t, TKey k, TData data);
TTable Delete(TTable t, TKey k);
bool Search(TTable t, TKey k);
TData GetData(TTable t, TKey k);

#endif