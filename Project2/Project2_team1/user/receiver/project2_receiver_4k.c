#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/io.h>
#include <fcntl.h>
#include <linux/types.h>
#include <sys/mman.h>
#include <signal.h>

#define BASEPORT 0x700
#define PAGE_SIZE (1 << 12)
#define BUFFER_SIZE (1 << 12)

void *shared_mem;
int out;
char *buf;

static void catch_function(int signo)
{
	/* receive data and copy to local buffer */
	memcpy(buf, shared_mem, BUFFER_SIZE);
	
	/* SEND ACK */
	outl(0,BASEPORT);
	out = 0;
}

int main()
{
	unsigned long h;
	buf = (char *)malloc(BUFFER_SIZE);
	if (!buf)
	{
		printf("buf failed\n");
		return 0;
	}
	/* init buffer */
	memset(buf, 0, BUFFER_SIZE);
	/* change the I/O privilege level */
	if (iopl(3))
	{
		printf("failed to iopl, errno: %d\n", errno);
		goto out3;
	}
	/* open the char dev of module */
	int fd = open("/dev/project2", O_RDWR);
	if (fd < 0)
	{
		printf("failed to open\n");
		goto out3;
	}
	/* register the signal handler */
	shared_mem = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shared_mem == MAP_FAILED) 
	{
		printf("mmap failed\n");
		goto out2;
	}
	/* register the signal handler */
	if (signal(SIGUSR1, catch_function) == SIG_ERR) {
		printf("An error occurred while setting a signal handler.\n");
		goto out1;
	}

	out = 1;
	/* waiting for interrupt */
	while(out){}
	
	/* open the file of output */
	int fd_output = open("output_4k", O_RDWR | O_CREAT);
	if (fd_output < 0)
	{
		printf("failed to open\n");
		goto out1;
	}
	if (write(fd_output, buf, BUFFER_SIZE)!= BUFFER_SIZE)
		printf("write failed\n");
	
	close(fd_output);

out1:
	munmap(shared_mem, BUFFER_SIZE);
out2:
	close(fd);
out3:
	free(buf);
	return 0;
}
