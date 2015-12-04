#include <mmu.h>
#include <semaphore.h>
#include <sched.h>

struct processQueue{
	int pid;
	struct processQueue	* next;	
};


typedef struct Semaphore{
	processQueue *	queue;
	int		value;
	processQueue *	last;
};
/*
CreateSem - aloca un semaforo y establece su cuenta inicial.
*/

Semaphore *CreateSem(unsigned value)
{
	Semaphore *sem = mmu_kmalloc(sizeof(Semaphore));
	sem->value = value;
	sem->queue = NULL;
	sem->last = NULL;
	return sem;
}

/*
DeleteSem - da de baja un semaforo.
*/

void DeleteSem(Semaphore *sem)
{
	//int ints = SetInts(false);
	FlushSemQueue(sem);
	mmu_kfree(sem);
	//SetInts(ints);
}

/*
WaitSem, WaitSemCond, WaitSemTimed - esperar en un semaforo.
WaitSem espera indefinidamente, WaitSemCond retorna inmediatamente y
WaitSemTimed espera con timeout. El valor de retorno indica si se consumio
un evento del semaforo.
*/

int WaitSem(Semaphore *sem)
{
	if(sem->value > 0){
		sem->value--;
		return 1;
	}
	struct processQueue* newProc = mmu_kmalloc(sizeof(struct processQueue));

	newProc->pid = getpid();
	newProc->next = NULL;

	if(sem->queue == NULL){

		sem->queue->pid = newProc->pid;
		sem->queue = newProc;
		sem->last = newProc;
	}
	else{
		sem->last->next = newProc;
		sem->last = newProc;
	}

	sleep(newProc->pid);
	if(sem->value > 0){
		sem->value--;
		return 1;
	}
	return 0;
}


/*
SignalSem - senaliza un semaforo.
Despierta a la primera tarea de la cola o incrementa la cuenta si la cola
esta vacia.
*/

void SignalSem(Semaphore *sem)
{

	if ( sem->queue == NULL ){
		sem->value++;
		return;
	}
	int pid = sem->queue->pid;
	sem->queue = sem->queue->next;
	wake(pid);
}

/*
ValueSem - informa la cuenta de un semaforo.
*/

unsigned ValueSem(Semaphore *sem)
{
	return sem->value;
}

/*
FlushSem - despierta todas las tareas que esperan en un semaforo.
Las tareas completan su WaitSem() con el status que se pasa como argumento.
Deja la cuenta en cero.
*/

void FlushSem(Semaphore *sem)
{
	sem->value = -1;
	//bool ints = SetInts(false);
	while(sem->queue != NULL){
		SignalSem(sem);
	}
	FlushSemQueue(sem);
}

void FlushSemQueue(Semaphore *sem)
{
	struct processQueue * aux;
	while(sem->queue != NULL){
		aux = sem->queue;
		sem->queue = sem->queue->next;
		mmu_kfree(aux);
	}
}