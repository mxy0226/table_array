/*#include <mem.h>*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "table_array.h"

TBL_HANDLE_ARRAY g_table_arraytble[GLOBAL_MAX_ARRAY_TABLE];
int g_curr_tblcount = 0;

#define CMPRESULT_BIGGER 1
#define CMPRESULT_SMALLER -1
#define CMPRESULT_EQUAL 0

static int binary_lookup(void *pIndexArray, int elementSize, const char * key,	int ksize,	int start, int end, int *iPos, int *cmpResult, int (*keycmp)(void *, int, void *, int));
/*------------- db operation ----------------*/
/****************************************************************************
* FUNCTION NAME: create_table_array
* DESCRIPTION:  register the table to the global table
* PARAMETERS:
    char    *tblname  table name
    void    *arrayaddr  the address which the array starts
    int     rowsize  the max size of a row
    int     maxrows  the array size
    int (*keycmp)(void *, int, void *, int)  the key comparation function
    int (*getkey)(void *, int, void *, int)  the get key function
    int entryCnt	the number of elements in data entry array
* RETURN VALUES: if succeed, return TBL_HANDLE_ARRAY pointer which has just been malloced,
                 if failed, return NULL
******************************************************************************/
TBL_HANDLE_ARRAY *create_table_array(char *tblname,
        void *arrayaddr, int rowsize, int maxrows, int currrows, int indexsize,
        int (*keycmp)(void *, int, void *, int),
        int (*getkey)(void *, int, void *, int),
        int (*putkey)(void *, int, void *, int),
		int (*defaultData)(),
		int (*getcli)(void *entry, int entsize, void *buf, int bufsize)
		)
{
    TBL_HANDLE_ARRAY handler;
	char *data, *index, **pDataHandler;
    int i = 0;
    int len;
    if (g_curr_tblcount == GLOBAL_MAX_ARRAY_TABLE)
    {
        return NULL;
    }

	memset(handler.tblname, 0, TBLNAME_MAXSIZE);
    /*check whether the tblname has existed or not*/
    for(; i < g_curr_tblcount; ++i)
    {
        if ( memcmp(g_table_arraytble[i].tblname, tblname, TBLNAME_MAXSIZE - 1) == 0)
        {
            break;
        }
    }
    if (i < g_curr_tblcount)
    {
        return NULL;
    }

    /* construct a new handler */
    if(strlen(tblname) < TBLNAME_MAXSIZE)
        strcpy(handler.tblname, tblname);
    else
    {
        memcpy(handler.tblname, tblname, TBLNAME_MAXSIZE - 1);
        handler.tblname[TBLNAME_MAXSIZE - 1] = 0;
    }

    handler.keycmp = keycmp;
    handler.getkey = getkey;
    handler.putkey = putkey;
	handler.defaultData = defaultData;
	handler.getcli = getcli;

    handler.accesstype = ARRAY;

    handler.maxrowsize = rowsize;
    handler.maxrows = maxrows;
    if (currrows < 0)
        handler.currrows = 0;
    else
        handler.currrows = currrows;

    handler.arrayaddr = arrayaddr;
	len = (maxrows + 8) / 8 ;

	handler.pBitmap = (char *)malloc( len );/* it must be freed while droping this table */
	if(handler.pBitmap == NULL)
		printf("create_table_array: %s bitmap malloc failed\n", tblname);
	memset(handler.pBitmap, 0xFF, len);

	handler.indexsize = indexsize;
	len = (indexsize + sizeof(void *)) * maxrows;
	handler.pIndex = (void *)malloc( len );/* it must be freed while droping this table */
	if(handler.pIndex == NULL)
		printf("create_table_array: %s pIndex malloc failed\n", tblname);

	memset(handler.pIndex, 0, len);

	if(currrows > 0)
	{
		int n = currrows / 8, r = currrows % 8;
		int j ;

		/* initiate the bitmap */
		memset(handler.pBitmap, 0, n);
		for(j = 0; j < r ; ++j)
			LSB_setBitsOff(handler.pBitmap[n], 8, j);

		/* initiate the index */
		for(j = 0; j < currrows; ++j)
		{
			data = ((char *)arrayaddr + rowsize * j);
			index = ((char *)handler.pIndex + (indexsize + sizeof(void *)) * j);
			pDataHandler = (char **)(index + indexsize);

			handler.getkey(data, rowsize, index, indexsize); /*get index*/
			*pDataHandler = (char *)data; /* setup the pHandler of the index */
		}
	}

    /*add sem_id to support the multiple access*/
#if (TBL_ARRAY_OS_VER == WINDOWS)
	handler.m_mutex = semMCreate(handler.tblname);
#else
#if(TBL_ARRAY_OS_VER == VXWORKS_OS)
    handler.m_mutex = semMCreate(SEM_Q_FIFO);
#else
#if(TBL_ARRAY_OS_VER == LINUX_OS)
    sem_init(&handler.m_mutex,0,1);     
#endif

#endif
#endif


    /* register to g_table_arraytble */
    memcpy(&g_table_arraytble[g_curr_tblcount], &handler, sizeof(TBL_HANDLE_ARRAY));

    return &g_table_arraytble[g_curr_tblcount++];
}

/****************************************************************************
* FUNCTION NAME: drop_table_array
* DESCRIPTION:  de-register the table from the gobal table,
* PARAMETERS:
    char    *tblname  table name
* RETURN VALUES: if succeed, return 0 , else return -1
******************************************************************************/
int  drop_table_array(TBL_HANDLE_ARRAY *htbl)
{
    int i = 0;
    for(; i < g_curr_tblcount; ++i)
    {
        if ( memcmp(g_table_arraytble[i].tblname, htbl->tblname, TBLNAME_MAXSIZE - 1) == 0)
        {
            break;
        }
    }

    if (i < g_curr_tblcount)
    {
        return -1;
    }
    else
    {
        if( i < g_curr_tblcount - 1)
        {
            TBL_HANDLE_ARRAY tmpTblTbl[GLOBAL_MAX_ARRAY_TABLE];

			free(htbl->pBitmap);  /*free the pBitmap*/
			htbl->pBitmap = NULL;
			free(htbl->pIndex); /*free the pIndex*/
			htbl->pIndex = NULL;

            i++;
            memcpy(tmpTblTbl, &g_table_arraytble[i], (g_curr_tblcount - i) * sizeof(TBL_HANDLE_ARRAY));
            memcpy(&g_table_arraytble[i - 1], tmpTblTbl, (g_curr_tblcount - i) * sizeof(TBL_HANDLE_ARRAY));
        }

        memset(&g_table_arraytble[g_curr_tblcount - 1], 0, sizeof(TBL_HANDLE_ARRAY));
        return 0;
    }
}

/****************************************************************************
* FUNCTION NAME: open_table_array
* DESCRIPTION:  open the table by giving the tblname
* PARAMETERS:
    char    *tblname  table name
* RETURN VALUES: if succeed, return TBL_HANDLE_ARRAY pointer which is related to tblname,
                 if failed, return NULL
******************************************************************************/
TBL_HANDLE_ARRAY *open_table_array(char *tblname)
{
    int i = 0;
    for(; i < g_curr_tblcount; ++i)
    {
        if ( memcmp(g_table_arraytble[i].tblname, tblname, TBLNAME_MAXSIZE - 1) == 0)
        {
            break;
        }
    }

    if (i < g_curr_tblcount)
    {
        return &g_table_arraytble[i];
    }
    else
    {
        return NULL;
    }
}

/****************************************************************************
* FUNCTION NAME: close_table_array
* DESCRIPTION:  close the table by giving the tblname
* PARAMETERS:
    char    *tblname  table name
* RETURN VALUES: if succeed, return 0,
                 if failed, return -1
******************************************************************************/
int *close_table_array(TBL_HANDLE_ARRAY *handle)
{
    int i = 0;
    for(; i < g_curr_tblcount; ++i)
    {
        if (memcmp(g_table_arraytble[i].tblname, handle->tblname, TBLNAME_MAXSIZE - 1)  == 0)
        {
            break;
        }
    }

	/*do nothing*/

	return 0;
}

/****************************************************************************
* FUNCTION NAME: get_entry_array
* DESCRIPTION:  get the entry
* PARAMETERS:
    TBL_HANDLE_ARRAY    *htbl  table handle
    void    *key  table index
    int    ksize  size of the key
    void    *buf  the output entry
    int    bufsize  the size of the buf
* RETURN VALUES: if succeed, return 0
                else if bufsize is not enough, return the necessary size(>0)
                else return -1(can not find the entry)
******************************************************************************/
int get_entry_array(TBL_HANDLE_ARRAY *htbl, void *key, int ksize, void *buf, int bufsize)
{
    int cmpResult=99999, i = 0;
    int start = 0, end = 0;
    int rc = -1;

#if (TBL_ARRAY_OS_VER == WINDOWS) || (TBL_ARRAY_OS_VER == VXWORKS_OS) || (TBL_ARRAY_OS_VER == LINUX_OS)
    semTake(htbl->m_mutex, INFINITE_DB);
#endif

    end = htbl->currrows;
	if(htbl->currrows == 0)
	{
		rc = -1;
	}
    else if (bufsize < htbl->maxrowsize)
    {
        rc = htbl->maxrowsize;
    }
    else
    {
        if(binary_lookup(htbl->pIndex, htbl->indexsize + sizeof(void *),key, ksize, start, end, &i, &cmpResult, htbl->keycmp) == NO_ERROR)
        {
			char **pDataHandler = (char **)( (char *)htbl->pIndex
											+ (htbl->indexsize + sizeof(void *)) * i
											+ htbl->indexsize );

            memcpy(buf, *pDataHandler, htbl->maxrowsize);
/*            rc = htbl->maxrowsize; */
            rc = 0;
        }
    }

#if (TBL_ARRAY_OS_VER == WINDOWS) || (TBL_ARRAY_OS_VER == VXWORKS_OS) || (TBL_ARRAY_OS_VER == LINUX_OS)
    semMGive(htbl->m_mutex);
#endif

    return rc;
}

/****************************************************************************
* FUNCTION NAME: next_entry_array
* DESCRIPTION:  get the next entry
* PARAMETERS:
    TBL_HANDLE_ARRAY    *htbl  table handle
    void    *key  table index
    int    ksize  size of the key
    void    *buf  the output entry
    int    bufsize  the size of the buf
* RETURN VALUES: if succeed, return 0 ,
                else if bufsize is not enough, return the necessary size(>0)
                else return -1(can not find the next entry)
******************************************************************************/
int next_entry_array(TBL_HANDLE_ARRAY *htbl, void *key, int ksize, void *buf, int bufsize)
{
    int cmpResult=99999, i = 0;
    int start = 0, end = 0;
    int rc = -1;

#if (TBL_ARRAY_OS_VER == WINDOWS) || (TBL_ARRAY_OS_VER == VXWORKS_OS) || (TBL_ARRAY_OS_VER == LINUX_OS)
    semTake(htbl->m_mutex, INFINITE_DB);
#endif

    end = htbl->currrows;

    if (bufsize < htbl->maxrowsize)
    {
        rc = htbl->maxrowsize;
    }
    else if(htbl->currrows <= 0)
    {
    	rc = -1;
    }
    else if (key == NULL)
    {
		char **pDataHandler = (char **)( (char *)htbl->pIndex + htbl->indexsize );

        memcpy(buf, *pDataHandler, htbl->maxrowsize);
/*      rc = htbl->maxrowsize; */
        rc = 0;
    }
    else
    {
        if ((binary_lookup(htbl->pIndex, htbl->indexsize + sizeof(void *), key, ksize, start, end, &i, &cmpResult, htbl->keycmp) == NO_ERROR)
				&& (i < htbl->currrows - 1)	)
        {
			char **pDataHandler = (char **)( (char *)htbl->pIndex
											+(htbl->indexsize + sizeof(void *)) * (i + 1)
											+ htbl->indexsize );

            memcpy(buf, *pDataHandler, htbl->maxrowsize);
/*          rc = htbl->maxrowsize;*/
            rc = 0;
        }
        else
        {
            int nextpos = i;
            rc = 0;
            switch (cmpResult) 
            {
                case CMPRESULT_SMALLER:
                    nextpos = i;
                    break;
                case CMPRESULT_BIGGER: 
                    nextpos = i + 1;
                    break;
                case CMPRESULT_EQUAL:
                default:
                    rc = -1;
            }
            if (0 == rc)
            {
                if (nextpos < htbl->currrows)
                {
                    char **pDataHandler = (char **)( (char *)htbl->pIndex
                                            +(htbl->indexsize + sizeof(void *)) * nextpos
                                            + htbl->indexsize );

                    memcpy(buf, *pDataHandler, htbl->maxrowsize);
                }else{
                    rc = -1;
                }
            }
        }
    }

#if (TBL_ARRAY_OS_VER == WINDOWS) || (TBL_ARRAY_OS_VER == VXWORKS_OS) || (TBL_ARRAY_OS_VER == LINUX_OS)
    semMGive(htbl->m_mutex);
#endif

    return rc;
}

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
                else return -4 (meet the max row count)
******************************************************************************/
int add_entry_array(TBL_HANDLE_ARRAY *htbl,  void *key, int ksize, void *entry, int entsize)
{
    int cmpResult=99999, i = 0, j = 0, k = 0;
    int start = 0, end = 0;
    int rc = NO_ERROR;
	char **pDataHandler = NULL;
	int iEmptyPosition = -1;

#if (TBL_ARRAY_OS_VER == WINDOWS) || (TBL_ARRAY_OS_VER == VXWORKS_OS) || (TBL_ARRAY_OS_VER == LINUX_OS)
    semTake(htbl->m_mutex, INFINITE_DB);
#endif

    end = htbl->currrows;

    if (htbl->maxrows == htbl->currrows)
    {
        rc = -4;
    }
    else if (entsize > htbl->maxrowsize)
    {
        rc = -1;
    }
    else if (htbl->currrows == 0)
    {
		i = 0;  /* the first element of the index array*/
		iEmptyPosition = 0; /* the first element of the data array*/
		j = 0; /* the first bit of the pBitmap*/
		k = 0;
    }
    else /*<!-- else 0 -->*/
    {
        if (binary_lookup(htbl->pIndex, htbl->indexsize + sizeof(void *), key, ksize, start, end, &i, &cmpResult, htbl->keycmp) == NO_ERROR)
        {
            rc = -3;
        }
#if 0
        else if (cmpResult == 0)/*has found the same record*/
        {
            rc = -2;
        }
#endif
        else /*<!-- else 1 -->*/
        {
			/* get an empty position from the htbl->pBitmap */
			int n = (htbl->maxrows + 8) / 8;

			for(j =0; j < n; ++j)
			{
				if (htbl->pBitmap[j] != 0)
				{
					for(k = 0; k < 8; ++k)
					{
						if(LSB_isBitEnabled(htbl->pBitmap[j], 8, k))/* find the non-0 bit */
							break;
					}
					break;
				}
			}

			iEmptyPosition = 8 * j + k;
        }/*<!-- else 1 -->*/
    }/*<!-- else 0 -->*/

	if(rc == NO_ERROR)
	{
		/* add key to htbl->pIndex */
		int elementSize = (htbl->indexsize + sizeof(void *));
		char * indexData = NULL;
#if 0
		if(cmpResult == CMPRESULT_SMALLER)/* It means the last compared index is smaller than the key,
											so the new record with key shall be inserted to the next position*/
#endif
		if(cmpResult == CMPRESULT_BIGGER)
		{
			i++ ;
		}

		indexData = (char *)htbl->pIndex + elementSize * i;
		if( i <= htbl->currrows - 1 )/*move the tail data to the next space*/
		{
			memmove(indexData + elementSize, indexData, elementSize * (htbl->currrows - i));
		}

		memset(indexData, 0, elementSize);
		memcpy(indexData, key, ksize);

		pDataHandler = (char **)( indexData + htbl->indexsize ); /* specify the pDataHandler */

		/* add entry to htbl->arrayaddr */
		*pDataHandler = (char *)htbl->arrayaddr + htbl->maxrowsize * iEmptyPosition;
		memset(*pDataHandler, 0, htbl->maxrowsize);
		htbl->putkey(entry, entsize, key, ksize);/*put key to the entry*/
		memcpy(*pDataHandler, entry, entsize);

		/*deal with the pBitmap*/
		LSB_setBitsOff(htbl->pBitmap[j], 8, k);

		/*increase the entry count*/
		htbl->currrows++;
	}

#if (TBL_ARRAY_OS_VER == WINDOWS) || (TBL_ARRAY_OS_VER == VXWORKS_OS) || (TBL_ARRAY_OS_VER == LINUX_OS)
    semMGive(htbl->m_mutex);
#endif

    return rc;
}

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
******************************************************************************/
int modify_entry_array(TBL_HANDLE_ARRAY *htbl, void *key, int ksize, void *entry, int entsize)\
{
    int cmpResult, i = 0;
    int start = 0, end = 0;
    int rc = -3;

#if (TBL_ARRAY_OS_VER == WINDOWS) || (TBL_ARRAY_OS_VER == VXWORKS_OS) || (TBL_ARRAY_OS_VER == LINUX_OS)
    semTake(htbl->m_mutex, INFINITE_DB);
#endif

    end = htbl->currrows;

    if (entsize > htbl->maxrowsize || htbl->currrows <= 0)
    {
        rc = -1;
    }
    else
    {
        if (binary_lookup(htbl->pIndex, htbl->indexsize + sizeof(void *), key, ksize, start, end, &i, &cmpResult, htbl->keycmp) == NO_ERROR)
        {
			char **pDataHandler = (char **)( (char *)htbl->pIndex
											+ (htbl->indexsize + sizeof(void *)) * i
											+ htbl->indexsize );

            memset(*pDataHandler, 0, htbl->maxrowsize);
            memcpy(*pDataHandler, entry, htbl->maxrowsize);

            rc = 0;
        }
        else
        {
            rc = -3;
        }
    }

#if (TBL_ARRAY_OS_VER == WINDOWS) || (TBL_ARRAY_OS_VER == VXWORKS_OS) || (TBL_ARRAY_OS_VER == LINUX_OS)
    semMGive(htbl->m_mutex);
#endif

    return rc;
}

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
int remove_entry_array(TBL_HANDLE_ARRAY *htbl, void *key, int ksize)
{
    int cmpResult=99999, i = 0;
    int start = 0, end = 0;
    int rc = -2;

#if (TBL_ARRAY_OS_VER == WINDOWS) || (TBL_ARRAY_OS_VER == VXWORKS_OS) || (TBL_ARRAY_OS_VER == LINUX_OS)
    semTake(htbl->m_mutex, INFINITE_DB);
#endif

    end = htbl->currrows;

    if(htbl->currrows <= 0)
    {
    	rc = -1;
    }
    else
    {
        if(binary_lookup(htbl->pIndex, htbl->indexsize + sizeof(void *), key, ksize, start, end, &i, &cmpResult, htbl->keycmp) == NO_ERROR)
        {
			int rightPosition = -1;
			int j = 0, k = 0;
			int elementSize = htbl->indexsize + sizeof(void *);
			char **pDataHandler = (char **)( (char *)htbl->pIndex
											+ (htbl->indexsize + sizeof(void *)) * i
											+ htbl->indexsize );

			/* get an right position of the table data array */
			rightPosition = (*pDataHandler - (char *)htbl->arrayaddr) / htbl->maxrowsize;

			/* get the bit position of pBitmap */
			j = rightPosition / 8;
			k = rightPosition % 8;

			/* remove the key from index array */
			if( i != htbl->currrows - 1)
			{
				char *dataIndex = (char *)htbl->pIndex + elementSize * i;
                            memmove(dataIndex, dataIndex + elementSize, (htbl->currrows - 1 - i) * elementSize);
			}

                    memset((char *)htbl->pIndex + (htbl->currrows - 1) * elementSize, 0, elementSize);

			LSB_setBitsOn(htbl->pBitmap[j], 8, k);
            htbl->currrows--;
            rc = 0;
        }
        else
        {
            rc = -2;
        }
    }

#if (TBL_ARRAY_OS_VER == WINDOWS) || (TBL_ARRAY_OS_VER == VXWORKS_OS) || (TBL_ARRAY_OS_VER == LINUX_OS)
    semMGive(htbl->m_mutex);
#endif

    return rc;
}

static int binary_lookup(void *pIndexArray, int elementSize,
						const char * key,	int ksize,
						int start, int end,
						int *iPos, int *cmpResult,
						int (*keycmp)(void *, int, void *, int))
{
	int low,mid,high,rc;
	char *data;
	int realIndexSize = elementSize - sizeof(void *);

	rc = -1;
	low  = 0;
	high = end-1;

    mid = 0;
    while(low <= high)
    {
        mid = (low + high) / 2;

        /* get key */
        data = (char *)((char*)pIndexArray + elementSize * mid);

        /* compare key */
        *cmpResult = keycmp((char *)key, ksize, data, realIndexSize);
        if (*cmpResult > 0)
        {
			*cmpResult = CMPRESULT_BIGGER;
			low = mid + 1;
        }
        else if (*cmpResult < 0)
        {
  			*cmpResult = CMPRESULT_SMALLER;
			high = mid -1;
        }
		else
        {
        	*cmpResult = CMPRESULT_EQUAL;
        	rc = NO_ERROR;
        	break;
        }
    }

	*iPos = mid;

	return rc;
}

