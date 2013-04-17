#ifndef _CD_SEMAPHOREH
#define _CD_SEMAPHOREH

#ifndef OS_NONE
    #define OS_NONE  0
#endif
    
#ifndef WINDOWS
    #define WINDOWS  1
#endif

#ifndef VXWORKS_OS
    #define VXWORKS_OS  2
#endif

#ifndef LINUX_OS
    #define LINUX_OS  3
#endif

#ifndef TBL_ARRAY_OS_VER
    #define TBL_ARRAY_OS_VER  LINUX_OS
#endif

#if (TBL_ARRAY_OS_VER == WINDOWS)
#include <wtypes.h>
#include <winbase.h>

typedef HANDLE SEM_ID;
#define false FALSE 
#define true TRUE 

#define TBL_ARRAY_MAX_SEMAPHORE_COUNT 0xFFFF
#define semCCreate(options, initialCount) CreateSemaphore(NULL, initialCount, TBL_ARRAY_MAX_SEMAPHORE_COUNT,"TBL_ARRAY_SEM")
#define semCGive(semId) ReleaseSemaphore(semId, 1, NULL)
#define semMCreate(mutexname) CreateEvent(NULL,FALSE, TRUE,NULL)
#define semMGive(semId) SetEvent(semId)
#define semTake(semId, timeOut) WaitForSingleObject(semId, timeOut)
#define semDelete(semId) CloseHandle(semId)
#define TBL_ARRAY_TIMEOUT WAIT_TIMEOUT
#endif /* _OS_VER == WINDOWS */

#if (TBL_ARRAY_OS_VER == VXWORKS_OS)
#include <vxworks.h>
#include <semLib.h>
#define TBL_ARRAY_TIMEOUT ERROR
#define INFINITE_DB -1
#define semMGive(semId) semGive(semId)
#define semCGive(semId) semGive(semId)
#endif  /* _OS_VER == VXWORKS_OS */


#if (TBL_ARRAY_OS_VER == LINUX_OS)
#include <sys/sem.h>
#include <semaphore.h>
typedef sem_t SEM_ID;
#define semTake(semId,timout) sem_wait(&semId)
#define semMGive(semId) sem_post (&semId)
#endif  /* _OS_VER == LINUX_OS */

#endif
