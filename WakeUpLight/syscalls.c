#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/uart.h>
#include <driverlib/rom.h>
#include <unistd.h>
#include <errno.h>
 
#undef errno
extern int errno;

register char * stack_ptr asm("sp");

char *__env[1] = { 0 };
char **environ = __env;

int _write(int file, char *ptr, int len);

void initialise_monitor_handles()
{
}

int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

void _exit (int status)
{
	_write(1, "exit\n\r", 6);
	_kill(status, -1);
	while (1) {}	/* Make sure we hang here */
}

int _write(int file, char *ptr, int len) {
    int i;

    switch (file) {
    case STDOUT_FILENO:
    case STDERR_FILENO:
        for(i=0; i<len; i++) {
        	ROM_UARTCharPut(UART0_BASE, ptr[i]);
        }
        break;
    default:
        errno = EBADF;
        return -1;
    }
    return len;
}

caddr_t _sbrk(int incr)
{
	extern unsigned long _start_heap;
	static char *heap_end;
	char *prev_heap_end;

	if (heap_end == 0)
		heap_end = (char*)&_start_heap;

	prev_heap_end = heap_end;
	if (heap_end + incr > stack_ptr)
	{
		// write(1, "Heap and stack collision\n", 25);
		// abort();
		errno = ENOMEM;
		return (caddr_t) -1;
	}

	heap_end += incr;

	return (caddr_t) prev_heap_end;
}

int _close(int file)
{
	return -1;
}


int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
	return 1;
}

int _lseek(int file, int ptr, int dir)
{
	return 0;
}

int _read(int file, char *ptr, int len) {
    int n;
    int num = 0;
    char c;

    switch (file) {
    case STDIN_FILENO:
        for (n = 0; n < len; n++) {
            do
            {
              c = UARTCharGetNonBlocking(UART0_BASE);
            } while (c == -1);

            *ptr++ = c;
            num++;
        }
        break;
    default:
        errno = EBADF;
        return -1;
    }
    return num;
}

int _open(char *path, int flags, ...)
{
	/* Pretend like we always fail */
	return -1;
}

int _wait(int *status)
{
	errno = ECHILD;
	return -1;
}

int _unlink(char *name)
{
	errno = ENOENT;
	return -1;
}

int _times(struct tms *buf)
{
	return -1;
}

int _stat(char *file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _link(char *old, char *new)
{
	errno = EMLINK;
	return -1;
}

int _fork(void)
{
	errno = EAGAIN;
	return -1;
}

int _execve(char *name, char **argv, char **env)
{
	errno = ENOMEM;
	return -1;
}
