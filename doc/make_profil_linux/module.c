#include <linux/sched.h>	// debug information for task_struct and its members
#include <linux/thread_info.h>	// debug information for thread_info structure

void do_nothing(void)
{
   struct thread_info* ti = NULL;
   (void) ti;
}
