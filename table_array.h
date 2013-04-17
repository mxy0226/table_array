#ifndef ARRAYOPERH
#define ARRAYOPERH

#include "my_sem.h"

/* table access type */
#define ARRAY 1
#define LINKEDLIST 2
#define HASH 3
#define AVL 4
#define BTREE 5

#ifndef NO_ERROR
#define NO_ERROR 0
#endif

#ifndef NULL
#define NULL 0
#endif


/*
 *	The LSB of the first byte shall correspond to the first data block of the arrayaddr.
 */
#define LSB_setBitsOn(bits, bitscount, num)    (bits |= (1 << (bitscount - num - 1)))

#define LSB_setBitsOff(bits, bitscount, num)    (bits &= ~(1 << (bitscount - num - 1)))

#define LSB_isBitEnabled(bits, bitscount, num)  ((bits & (1 << (bitscount - num - 1))) != 0)


/* structure of table handle */
#define TBLNAME_MAXSIZE 32
typedef struct _TBL_HANDLE_HEADER {
    char tblname[TBLNAME_MAXSIZE];
    int (*keycmp)(void *, int, void *, int); /* void key1*, int ksize1, void *key2, int ksize2 */
    int (*getkey)(void *, int, void *, int); /* void entry, int entsize, void *key, int *ksize */
    int accesstype;
} TBL_HANDLE_HEADER;

typedef struct _TBL_HANDLE_ARRAY {
    char tblname[TBLNAME_MAXSIZE];/*table name*/
    int (*keycmp)(void *key1, int ksize1, void *key2, int ksize2);
    int (*getkey)(void *entry, int entsize, void *key, int ksize); 
    int (*putkey)(void *entry, int entsize, void *key, int ksize); 
    int (*defaultData)();
    int (*getcli)(void *entry, int entsize, void *buf, int bufsize); 
    int accesstype; /* It means the table data storage structure. It is not used now.*/
    int maxrowsize;  /* record size */
    int maxrows;     /* max record count */
    int currrows;    /* current record count */
    sem_t m_mutex;  /* multiple access mutex*/
	int indexsize;	/* index size */
    char *pBitmap;/*bitmap of the arrayaddr*/
    void *pIndex; /* index data pointer*/
    void *arrayaddr; /* data pointer*/
} TBL_HANDLE_ARRAY;

/* global table table */
#define GLOBAL_MAX_ARRAY_TABLE 128
extern TBL_HANDLE_ARRAY g_table_arraytble[GLOBAL_MAX_ARRAY_TABLE];
extern int g_curr_tblcount;

/* db operation */
/****************************************************************************
* FUNCTION NAME: create_table_array
* DESCRIPTION:  construct a handler and register the handler to the global table
* PARAMETERS:
    char    *tblname  table name
    void    *arrayaddr  the address which the array starts
    int     rowsize  the max size of a row
    int     maxrows  the array size    
    int (*keycmp)(void *, int, void *, int)  the key comparation function
    int (*getkey)(void *, int, void *, int)  the get key function
    int (*putkey)(void *, int, void *, int)  the put key function
    int entryCnt	the number of elements in data entry array
* RETURN VALUES: if succeed, return TBL_HANDLE_ARRAY pointer which has just been malloced,
                 if failed, return NULL
******************************************************************************/
extern TBL_HANDLE_ARRAY *create_table_array(char *tblname,
        void *arrayaddr, int rowsize, int maxrows, int currrows, int indexsize,
        int (*keycmp)(void *, int, void *, int),
        int (*getkey)(void *, int, void *, int),
		int (*putkey)(void *, int, void *, int),
		int (*defaultData)(),
		int (*getcli)(void *entry, int entsize, void *buf, int bufsize)
		);

/****************************************************************************
* FUNCTION NAME: drop_table_array
* DESCRIPTION:  de-register the handler from the gobal table, and destruct the handler
* PARAMETERS:
    char    *tblname  table name
* RETURN VALUES: if succeed, return 0 , else return -1
******************************************************************************/
extern int  drop_table_array(TBL_HANDLE_ARRAY *htbl);

/****************************************************************************
* FUNCTION NAME: open_table_array
* DESCRIPTION:  open the table by giving the tblname
* PARAMETERS:
    char    *tblname  table name
* RETURN VALUES: if succeed, return TBL_HANDLE_ARRAY pointer which is related to tblname,
                 if failed, return NULL
******************************************************************************/
extern TBL_HANDLE_ARRAY *open_table_array(char *tblname);

/****************************************************************************
* FUNCTION NAME: close_table_array
* DESCRIPTION:  close the table by giving the tblname
* PARAMETERS:
    TBL_HANDLE_ARRAY *handle	handler of the specific table
* RETURN VALUES: if succeed, return 0,
                 if failed, return -1
******************************************************************************/
extern int *close_table_array(TBL_HANDLE_ARRAY *handle);

/****************************************************************************
* FUNCTION NAME: get_entry_array
* DESCRIPTION:  get the entry
* PARAMETERS:
    TBL_HANDLE_ARRAY    *htbl  table handle
    void    *key  table index
    int    ksize  size of the key
    void    *buf  the output entry
    int    bufsize  the size of the buf
* RETURN VALUES: if succeed, return the real size of the entry ,
                else if bufsize is not enough, return the necessary size(>0)
                else return -1(can not find the entry)
******************************************************************************/
extern int get_entry_array(TBL_HANDLE_ARRAY *htbl, void *key, int ksize, void *buf, int bufsize);

/****************************************************************************
* FUNCTION NAME: next_entry_array
* DESCRIPTION:  get the next entry
* PARAMETERS:
    TBL_HANDLE_ARRAY    *htbl  table handle
    void    *key  table index
    int    ksize  size of the key
    void    *buf  the output entry
    int    bufsize  the size of the buf
* RETURN VALUES: if succeed, the real size of the entry ,
                else if bufsize is not enough, return the necessary size(>0)
                else return -1(can not find the next entry)
******************************************************************************/
extern int next_entry_array(TBL_HANDLE_ARRAY *htbl, void *key, int ksize, void *buf, int bufsize);

/****************************************************************************
* FUNCTION NAME: add_entry_array
* DESCRIPTION:  add the entry
* PARAMETERS:
    TBL_HANDLE_ARRAY    *htbl  table handle
    void    *key  table index
    int    ksize  size of the key
    void    *entry  the new entry to be added
    int    entsize  the size of the buf
* RETURN VALUES: if succeed, return 0 ,
                else if entsize is bigger than the max row size, return -1
                else return -2 (can not add )
                else return -3 (the entry exists)
******************************************************************************/
extern int add_entry_array(TBL_HANDLE_ARRAY *htbl,  void *key, int ksize, void *entry, int entsize);
/****************************************************************************
* FUNCTION NAME: modify_entry_array
* DESCRIPTION:  modify the entry
* PARAMETERS:
    TBL_HANDLE_ARRAY    *htbl  table handle
    void    *key  table index
    int    ksize  size of the key
    void    *entry  the new entry to be added
    int    entsize  the size of the buf
* RETURN VALUES: if succeed, return 0
                else if entsize is bigger than the max row size, return -1
                else return -2(can not modify, e.g. due to foreign key constraints)
                else return -3(can not find the entry)
                else return -4 (meet the max row count)
******************************************************************************/
extern int modify_entry_array(TBL_HANDLE_ARRAY *htbl, void *key, int ksize, void *entry, int entsize);

/****************************************************************************
* FUNCTION NAME: remove_entry_array
* DESCRIPTION:  remove the entry
* PARAMETERS:
    TBL_HANDLE_ARRAY    *htbl  table handle
    void    *key  table index
    int    ksize  size of the key
* RETURN VALUES: if succeed, return 0 ,
                else return -1 (can not remove, e.g. due to foreign key constraints)
                return -2 (can not find the entry)
******************************************************************************/
extern int remove_entry_array(TBL_HANDLE_ARRAY *htbl, void *key, int ksize);


#endif
