#include <linux/sched.h>		// task_struct
#include <linux/thread_info.h>		// thread_info
#include <linux/fs.h>			// file, path
#include <linux/dcache.h>		// qstr

void do_nothing(void)
{
   struct thread_info* ti = NULL;
   (void) ti;
}
