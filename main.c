#include "table_array.h"
#include "stdio.h"
#include "string.h"


 typedef struct{
    int wsStdTestID; /*not-accessible*/
    int wsStdTest; /*not-accessible*/
 }STRUCT_wsTestEntry_index;
 typedef struct{
    int wsStdTestID; /*not-accessible*/
    int wsStdTest; /*not-accessible*/
    int wsDate; /*read-create*/
 }STRUCT_wsTestEntry;
#define INDEX_CMP_RESULT_BIGGER 1
#define INDEX_CMP_RESULT_SMALLER -1
#define INDEX_CMP_RESULT_EQUAL 0

//global array
STRUCT_wsTestEntry g_wsTestEntry[100];
TBL_HANDLE_ARRAY *wsTestEntry_handle = NULL;

int wsTestEntry_getkey(void *entry, int entsize, void *buf, int bufsize);
int wsTestEntry_putkey(void *entry, int entsize, void *key, int ksize);
int wsTestEntry_defaultData();
int wsTestEntry_getcli(void *entry, int entsize, void *buf, int bufsize);
int int_keycmp(void *key1, int ksize1, void *key2, int ksize2);
int int_int_keycmp(void *key1, int ksize1, void *key2, int ksize2);

int main()
{
    STRUCT_wsTestEntry          extEntry;
    STRUCT_wsTestEntry_index    extIndex;
    STRUCT_wsTestEntry_index    extIndex2;

    memset(g_wsTestEntry, 0, 100*sizeof(STRUCT_wsTestEntry));
    wsTestEntry_handle = create_table_array("wsTestEntry",\
                                  g_wsTestEntry, sizeof(STRUCT_wsTestEntry), 100, 0, sizeof(STRUCT_wsTestEntry_index),\
                                  int_int_keycmp, wsTestEntry_getkey, wsTestEntry_putkey, wsTestEntry_defaultData, wsTestEntry_getcli);

    extIndex.wsStdTestID = 0;
    extIndex.wsStdTest = 0;
    extEntry.wsDate = 1;
    int rc = add_entry_array(wsTestEntry_handle, &extIndex, sizeof(STRUCT_wsTestEntry_index), &extEntry, sizeof(STRUCT_wsTestEntry));
    printf("add new entry : %d,%d , return %d\n",extEntry.wsStdTestID,extEntry.wsStdTest,rc);

    extIndex.wsStdTestID = 30;
    extIndex.wsStdTest = 8000;
    extEntry.wsDate = 2;
    rc = add_entry_array(wsTestEntry_handle, &extIndex, sizeof(STRUCT_wsTestEntry_index), &extEntry, sizeof(STRUCT_wsTestEntry));
    printf("add new entry : %d,%d , return %d\n",extEntry.wsStdTestID,extEntry.wsStdTest,rc);

    extIndex.wsStdTestID = 2;
    extIndex.wsStdTest = 1;
    extEntry.wsDate = 2;
    rc = add_entry_array(wsTestEntry_handle, &extIndex, sizeof(STRUCT_wsTestEntry_index), &extEntry, sizeof(STRUCT_wsTestEntry));
    printf("add new entry : %d,%d , return %d\n",extEntry.wsStdTestID,extEntry.wsStdTest,rc);

    extIndex.wsStdTestID = 1;
    extIndex.wsStdTest = 1;
    extEntry.wsDate = 26;
    rc = add_entry_array(wsTestEntry_handle, &extIndex, sizeof(STRUCT_wsTestEntry_index), &extEntry, sizeof(STRUCT_wsTestEntry));
    printf("add new entry : %d,%d , return %d\n",extEntry.wsStdTestID,extEntry.wsStdTest,rc);

    extIndex.wsStdTestID = 1;
    extIndex.wsStdTest = 1;
    rc = get_entry_array(wsTestEntry_handle, &extIndex, sizeof(STRUCT_wsTestEntry_index), &extEntry, sizeof(STRUCT_wsTestEntry));
    printf("get entry : %d,%d,%d , return %d\n",extEntry.wsStdTestID,extEntry.wsStdTest,extEntry.wsDate,rc);



#if 0 
    //add item
    extIndex.wsStdTestID = 2;
    extIndex.wsStdTest = 1;
    extEntry.wsDate = 2;
    rc = add_entry_array(wsTestEntry_handle, &extIndex, sizeof(STRUCT_wsTestEntry_index), &extEntry, sizeof(STRUCT_wsTestEntry));
    
    //modify item 一定要将entry的结构体填写完整
    extIndex.wsStdTestID = extEntry.wsStdTestID =  30;
    extIndex.wsStdTest = extEntry.wsStdTest = 8000;
    extEntry.wsDate = 20;
    modify_entry_array(wsTestEntry_handle, &extIndex, sizeof(STRUCT_wsTestEntry_index), &extEntry, sizeof(STRUCT_wsTestEntry));
    
    // del item
    extIndex.wsStdTestID = 2;
    extIndex.wsStdTest = 1;
    remove_entry_array(wsTestEntry_handle,&extIndex,sizeof(extIndex));
#endif


    show();
    return 1;
}
    

void show()
{
    int  rc;
    char str[164];
    STRUCT_wsTestEntry          extEntry;
    STRUCT_wsTestEntry_index    extIndex;

    rc = next_entry_array(wsTestEntry_handle, NULL, 0, &extEntry, sizeof(extEntry));
    if(rc == 0)
    {
        printf("entry: %d %d %d, return %d\r\n",extEntry.wsStdTestID,extEntry.wsStdTest,extEntry.wsDate,rc);
        extIndex.wsStdTestID = extEntry.wsStdTestID;
        extIndex.wsStdTest = extEntry.wsStdTest;
    } else
        return;


    while(next_entry_array(wsTestEntry_handle, &extIndex, sizeof(extIndex), &extEntry, sizeof(extEntry)) == 0) {
        printf("entry: %d %d %d, return %d\r\n",extEntry.wsStdTestID,extEntry.wsStdTest,extEntry.wsDate,rc);
        extIndex.wsStdTestID = extEntry.wsStdTestID;
        extIndex.wsStdTest = extEntry.wsStdTest;
        sleep(1);
    }

    return ;
}

int int_int_keycmp(void *key1, int ksize1, void *key2, int ksize2)
{
    typedef struct _int_index
    {
        int id1;
        int id2;
    } int_index_t;

    if (ksize1 < sizeof(int_index_t))
        return INDEX_CMP_RESULT_SMALLER;

    if (ksize2 < sizeof(int_index_t))
        return INDEX_CMP_RESULT_BIGGER;


    {
        int_index_t *index1, *index2;
        index1 = (int_index_t *)key1;
        index2 = (int_index_t *)key2;

        if ((index1->id1 > index2->id1) ||(index1->id1 == index2->id1 && index1->id2 > index2->id2))
         return INDEX_CMP_RESULT_BIGGER;
        else if (index1->id1 < index2->id1 ||(index1->id1 == index2->id1 && index1->id2 < index2->id2))
            return INDEX_CMP_RESULT_SMALLER;
        else
            return INDEX_CMP_RESULT_EQUAL;

#if 0
        if (index1->id1 > index2->id1)
            return INDEX_CMP_RESULT_BIGGER;
        else if (index1->id1 < index2->id1)
            return INDEX_CMP_RESULT_SMALLER;
        else
        {
            if (index1->id2 > index2->id2)
                return INDEX_CMP_RESULT_BIGGER;
            else if (index1->id1 < index2->id1)
                return INDEX_CMP_RESULT_SMALLER;
            else
                return INDEX_CMP_RESULT_EQUAL;
        }
#endif
    }

}

int int_keycmp(void *key1, int ksize1, void *key2, int ksize2)
{
    typedef struct _int_index
    {
        int id1;
    } int_index_t;

    if (ksize1 < sizeof(int_index_t))
        return INDEX_CMP_RESULT_SMALLER;

    if (ksize2 < sizeof(int_index_t))
        return INDEX_CMP_RESULT_BIGGER;

    {
        int_index_t *index1, *index2;
        index1 = (int_index_t *)key1;
        index2 = (int_index_t *)key2;

        if (index1->id1 > index2->id1)
            return INDEX_CMP_RESULT_BIGGER;
        else if (index1->id1 < index2->id1)
            return INDEX_CMP_RESULT_SMALLER;
        else
            return INDEX_CMP_RESULT_EQUAL;
    }
}

int wsTestEntry_getkey(void *entry, int entsize, void *buf, int bufsize)
{
    STRUCT_wsTestEntry *data = (STRUCT_wsTestEntry*)entry;

    if(entsize< sizeof(STRUCT_wsTestEntry))
        return -1;

    if(bufsize >= sizeof(STRUCT_wsTestEntry_index))
    {
        memcpy(buf, data, sizeof(STRUCT_wsTestEntry_index));
        return sizeof(STRUCT_wsTestEntry_index);
    }

    return -1;
}

int wsTestEntry_putkey(void *entry, int entsize, void *key, int ksize)
{
    STRUCT_wsTestEntry *data = (STRUCT_wsTestEntry*)entry;

    if(entsize < sizeof(STRUCT_wsTestEntry))
        return -1;

    if(ksize > sizeof(STRUCT_wsTestEntry_index))
        return -1;

    memcpy(data, key, ksize);

    return 0;
}
int wsTestEntry_defaultData()
{
    /* !!! 添加你的默认数据*/

    return 0;
}
int wsTestEntry_getcli(void *entry, int entsize, void *buf, int bufsize)
{
    
    return 0;

}    
