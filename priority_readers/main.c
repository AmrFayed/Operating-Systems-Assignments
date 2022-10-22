#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

/* CONSTANTS =============================================================== */
#define NUM_READERS 5
#define NUM_READS 5
#define NUM_WRITERS 5
#define NUM_WRITES 5
/* GLOBAL SHARED DATA ====================================================== */
unsigned int gSharedValue = 0;
int gWaitingReaders = 0, gReaders = 0;

CRITICAL_SECTION gSharedMemoryLock;
CONDITION_VARIABLE gReadPhase;
CONDITION_VARIABLE gWritePhase;
void *readerMain(void*);
void *writerMain(void*);

int main()
{

	int i;

	int readerNum[NUM_READERS];
	int writerNum[NUM_WRITERS];

	HANDLE  readerThreads[NUM_READERS] = { NULL }; // Handles for created threads
	HANDLE  writerThreads[NUM_WRITERS] = { NULL }; // Handles for created threads


	// Initial
    InitializeConditionVariable (&gReadPhase);
    InitializeConditionVariable (&gWritePhase);

    InitializeCriticalSection (&gSharedMemoryLock);

//    //create mutex
//    gSharedMemoryLock = CreateMutexW( CreateMutex(
//        NULL,              // default security attributes
//        FALSE,             // initially not owned
//        NULL);             // unnamed mutex
//    if (gSharedMemoryLock == NULL)
//    {
//        printf("CreateMutex error: %d\n", GetLastError());
//        return 1;
//    }

    // Start the readers
	for(i = 0; i < NUM_READERS; i++) {
		readerNum[i] = i;
		readerThreads[i] = (HANDLE)_beginthread(readerMain, 0, &readerNum[i]);
	}
	// Start the writers
	for(i = 0; i < NUM_WRITERS; i++) {
		writerNum[i] = i;
		writerThreads[i] =(HANDLE)_beginthread(writerMain, 0, &writerNum[i]);
	}

	// Wait on readers to finish
	WaitForMultipleObjects(NUM_READERS,readerThreads,1,INFINITE);

	// Wait on writers to finish
    WaitForMultipleObjects(NUM_WRITERS,writerThreads,1,INFINITE);

    // Close thread and mutex handles
    for( i=0; i < NUM_READERS; i++ )
        CloseHandle(readerThreads[i]);

    for( i=0; i < NUM_WRITERS; i++ )
        CloseHandle(writerThreads[i]);

    getch();
    return 0;
}
void *readerMain(void *threadArgument) {

	int id = *((int*)threadArgument);
	int i = 0, numReaders = 0;

	for(i = 0; i < NUM_READS; i++) {
		// Wait so that reads and writes do not all happen at once
	  usleep(2000);

		// Enter critical section
        EnterCriticalSection (&gSharedMemoryLock);
            gWaitingReaders++;
            while (gReaders == -1)
                SleepConditionVariableCS (&gReadPhase, &gSharedMemoryLock,INFINITE);
            gWaitingReaders--;
            numReaders = ++gReaders;
        LeaveCriticalSection(&gSharedMemoryLock);

	  // Read data
        fprintf(stdout, "[r%d] reading %u  [readers: %2d]\n", id, gSharedValue, numReaders);

	  // Exit critical section
        EnterCriticalSection(&gSharedMemoryLock);
            gReaders--;
            if (gReaders == 0)
                WakeConditionVariable (&gWritePhase);
        LeaveCriticalSection(&gSharedMemoryLock);
	}
}
void *writerMain(void *threadArgument) {

	int id = *((int*)threadArgument);
	int i = 0, numReaders = 0;

	for(i = 0; i < NUM_WRITES; i++) {
	  // Wait so that reads and writes do not all happen at once
	  usleep(2000);

		// Enter critical section
	  EnterCriticalSection(&gSharedMemoryLock);
	  	while (gReaders != 0) {
	  		SleepConditionVariableCS(&gWritePhase, &gSharedMemoryLock,INFINITE);
	  	}
	  	gReaders = -1;
	  	numReaders = gReaders;
	  LeaveCriticalSection(&gSharedMemoryLock);

	  // Write data
	  fprintf(stdout, "[w%d] writing %u* [readers: %2d]\n", id, ++gSharedValue, numReaders);

	  // Exit critical section
	  EnterCriticalSection(&gSharedMemoryLock);
	  	gReaders = 0;
	  	if (gWaitingReaders > 0) {
	  		WakeAllConditionVariable(&gReadPhase);
	  	}
	  	else {
	  		WakeConditionVariable(&gWritePhase);
	  	}
	  LeaveCriticalSection(&gSharedMemoryLock);
  }
}
