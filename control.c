//written by Ramsey Alsheikh
//this is the system call to run my implementation of the control algorithm

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <asm-generic/barrier.h>
#include <linux/resource.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/completion.h>

//reprents a control entry
typedef struct _control_entry
{
	struct list_head list;
	struct task_struct *task;
	int time;
} control_entry;

//return size of delta queue
int size(struct list_head *head)
{
	struct list_head *pos;
	int size = 0;
	list_for_each(pos, head)
	{
		size++;
	}
	return size;
}

//a function to put the created processes created to sleep for an indefinite amount of time
int napb(void* data)
{
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule();
	return 0;
}

asmlinkage long sys_control(void)
{
	int i;
	control_entry entries[100];
	control_entry *pos;
	control_entry head;
	INIT_LIST_HEAD(&head.list);
	head.time = NULL;

	int sleep_times[] = {169, 280, 560, 602, 714, 1018, 1290, 1451, 1506, 1532, 1584, 1690, 1706, 2229, 2309, 2432, 2476, 2499, 2554, 2559, 2567, 2659, 2755, 2822, 2899, 3129, 3147, 3176, 3333, 3365, 3427, 3485, 3618, 3620, 3703, 3845, 3970, 4020, 4431, 4472, 4505, 4676, 4677, 4784, 4789, 4944, 4975, 5082, 5108, 5154, 5208, 5237, 5359, 5527, 5700, 5842, 5860, 5924, 6067, 6169, 6173, 6250, 6274, 6332, 6353, 6566, 6597, 6674, 6790, 6847, 6883, 7148, 7214, 7317, 7400, 7435, 7547, 7640, 8031, 8089, 8102, 8152, 8383, 8448, 8500, 8515, 8735, 8778, 8799, 8881, 8887, 8934, 9046, 9075, 9254, 9266, 9462, 9568, 9927, 9930};

	//create control entries
	for (i = 0; i < 100; i++)
	{
		INIT_LIST_HEAD(&entries[i].list);
		entries[i].time = sleep_times[i];
		entries[i].task = kthread_run(napb, NULL, "control entry for inital %d", sleep_times[i]);
		list_add(&entries[i].list, &head.list);
	}

	//main loop
	do
	{
		mdelay(1);
		list_for_each_entry(pos, &head.list, list)
		{
			pos->time -= 1;
			if (pos->time <= 0)
			{
				//printk("REMOVING");
				pos->list.prev->next = pos->list.next;
				pos->list.next->prev = pos->list.prev;
				wake_up_process(pos->task);
			}
		}
	}
	while (size(&head.list) > 0);

	return 0;
}

