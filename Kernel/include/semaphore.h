#ifndef __semaphore
#define __semaphore 1

typedef struct processQueue processQueue;
typedef struct Semaphore Semaphore ;


Semaphore *CreateSem(unsigned value);
void DeleteSem(Semaphore *sem);
int WaitSem(Semaphore *sem);
void SignalSem(Semaphore *sem);
unsigned ValueSem(Semaphore *sem);
void FlushSem(Semaphore *sem);
void FlushSemQueue(Semaphore *sem);



#endif