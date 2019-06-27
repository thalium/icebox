// --------------------------------------------------------------
// linux_tst_ibx - needed binary to test Icebox linux osHelper
//
// /!\ NEED TO BE BUILT IN 32 BITS MODE /!\
// It allows Icebox to correctly test the reading of process flags
//
// /!\ NEED TO BE NAMED 'linux_tst_ibx' /!\
// It allows Icebox to correctly find it
//
// It may need to install gcc-multilib before running it
//
// --------------------------------------------------------------

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define ARG "child"

void *runner(void *ptr)
{
	struct timespec delay;

	printf("Thread running\n");
	
	delay.tv_sec = 1;
	delay.tv_nsec = 0;

	while (1)
		nanosleep(&delay, NULL);
}

int main(int argc, char *argv[])
{
	int len_path;
	char* child_path;
	pthread_t thread;
	
	if (argc <= 1 || strcmp(argv[1], ARG) != 0)
	{
		printf("Parent running\n");
		len_path = strlen(argv[0]) + 1 + strlen(ARG) + 1;
		child_path = (char*) malloc(len_path * sizeof(char));
		snprintf(child_path, len_path, "%s %s", argv[0], ARG);
		return system(child_path);
	}
	else
		printf("Child running\n");
	
	pthread_create(&thread, NULL, runner, NULL);
	pthread_join(thread, NULL);
	
	return -1;
}