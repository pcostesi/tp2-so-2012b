#include <semaphore.h>
#include <mmu.h>

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


void OpenPipe(int fd)
{
	if(pipes[fd] != NULL){
		pipes[fd]->users++;
		return;
	}
	struct Pipe *p = mmu_kmalloc(sizeof(struct Pipe));
	p->head = p->tail = p->buf = mmu_kmalloc(p->size = PIPE_SIZE);
	p->end = p->buf + PIPE_SIZE;
	p->users = 1;
	p->sem = CreateSem(1);
	pipes[fd] = p;
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

	if(!WaitSem(p->sem)){
		return -1;
	}
	if(p->avail < size){
		//wait
	}
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