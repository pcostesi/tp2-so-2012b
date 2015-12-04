#include <semaphore.h>
#include <mmu.h>
#include <sched.h>

#define MAX_PIPES 20
#define PIPE_SIZE 200

static struct Pipe* pipes[MAX_PIPES];


struct Pipe
{
	Semaphore * 	sem;
	unsigned		size;
	unsigned		avail;
	char *			buf;
	char *			head;
	char *			tail;
	char *			end;
	int 			users;
}; 


int OpenPipe(int fd)
{
	if(fd >= MAX_PIPES){
		return -1;
	}
	if(pipes[fd] != NULL){
		pipes[fd]->users++;
		return fd;
	}
	struct Pipe *p = mmu_kmalloc(sizeof(struct Pipe));
	p->head = p->tail = p->buf = mmu_kmalloc(p->size = PIPE_SIZE);
	p->end = p->buf + PIPE_SIZE;
	p->users = 1;
	p->sem = CreateSem(1);
	pipes[fd] = p;
	return fd;
}

void DeletePipe(int fd)
{
	struct Pipe *p = pipes[fd];
	if(p==NULL){
		return;
	}
	FlushSem(p->sem);
	FlushSemQueue(p->sem);
	DeleteSem(p->sem);
	mmu_kfree(p);
	pipes[fd] = NULL;
}


/*Si no hay nada que leer, devuelve*/
int GetPipe(int fd, void *data, unsigned size)
{
	struct Pipe *p = pipes[fd];
	if( p == NULL){
		return -1;
	}

	if(size < 1 || size > p->size){
		return 0;
	}

	if(!WaitSem(p->sem)){
		return -1;
	}

	//read
	int read = 0;
	char* d = data;
	while( size > read && p->head != p->tail && p->avail){
		*d++ = *p->head++;
		if(p->head == p->end){
			p->head = p->buf;
		}
		read--;
		p->avail--;
	}

	SignalSem(p->sem);

	return read;

}



/*Si no hay lugar para escribir devuelve*/
int PutPipe(int fd, void *data, unsigned size)
{
	struct Pipe *p = pipes[fd];
	if( p == NULL){
		return -1;
	}

	if(size > p->size)
	{
		return 0;
	}

	if(p->avail < size){
		return 0;
	}
	/*while(p->avail < size){
		wait(sched_getpid(),500);
		if(!WaitSem(p->sem)){
		return -1;
	}	
	*/
	
	
	char* d = data;
	while(size > 0){
		*p->head++ = *d++;
		if(p->head == p->end){
			p->head = p->buf;
		}
		size--;
		p->avail++;
	}

	SignalSem(p->sem);
	return size;
}

void ClosePipe(int fd)
{
	pipes[fd]->users--;
	if(pipes[fd]->users){
		DeletePipe(fd);
	}
}


int syscall_opipe(int fd)
{	
	return OpenPipe(fd);
} 
void syscall_cpipe(int fd)
{
	ClosePipe(fd);
}
int syscall_wpipe(int fd, void* data, unsigned int size)
{
	return PutPipe(fd, data, size);
}
int syscall_rpipe (int fd, void* data, unsigned int size)
{
	return GetPipe(fd, data, size);
}