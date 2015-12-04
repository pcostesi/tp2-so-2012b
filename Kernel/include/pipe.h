#ifndef __pipe
#define __pipe 1




void OpenPipe(int fd);
void DeletePipe(int fd);
int GetPipe(int fd, void *data, unsigned size);
int PutPipe(int fd, void *data, unsigned size);
void ClosePipe(int fd);

#endif