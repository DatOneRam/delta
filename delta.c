//written by Ramsey Alsheikh
//this is the system call to run my implementation of the delta queue

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

//modified list_for_each_entry macro that goes backwards and is modified for this experiemnt
#define list_for_each_entry_backwards(pos, head, member)                          \
         for (pos = list_entry((head)->prev, typeof(*pos), member);      \
              &pos->member != (head) && init > sum;        \
              pos = list_entry(pos->member.prev, typeof(*pos), member)) 

//modified list_for_each_entry_backwards to start from a specifed entry
#define list_for_each_entry_backwards_from(pos, head, member)                          \
         for (pos = list_entry(&pos->member, typeof(*pos), member);      \
              &pos->member != (head) && init > sum;        \
              pos = list_entry(pos->member.prev, typeof(*pos), member)) 

//an entry in the delta queue is represented as type "delta_entry"
typedef struct _delta_entry
{
	struct task_struct *task;
	struct list_head list;
	int delta_time;
} delta_entry;

//method to add a delta_entry into the delta queue
//if there are no entries in the delta queue as of yet, dont use this, use the list_add that comes with the kernel
void dq_add(delta_entry *new, int init, delta_entry* head)
{
	 delta_entry* pos;
	 int sum = 0;
	
	 //add up the delta times before the time we want to add
         pos = list_entry(head->list.prev, delta_entry, list);
	 action:
	 sum += pos->delta_time;
	 if (pos != head && init > sum)
	 {
	 	pos = list_entry(pos->list.prev, delta_entry, list);
		//uncomment for debugging purposes
		//printk("dq_add: sum = %d, init = %d", sum, init);
	        goto action;
	 }
	
	 //calcuate the delta_time for our new entry
	 sum -= pos->delta_time;
	 new->delta_time = init - sum;
	
	//add our new entry at the appropiate spot
	__list_add(&new->list, &pos->list, &list_entry(pos->list.next, delta_entry, list)->list);
	
	//update the entry behind it so that it maintains its total time
	if(new->list.prev != head)
	{
		list_entry(new->list.prev, delta_entry, list)->delta_time -= new->delta_time;
	}                          
}

//method to lower the delta time of the root by one, and if needed remove it
//needs the head of the delta queue
void dq_update(struct list_head *delta_queue)
{
    //find and store the root
    delta_entry *root = list_entry(delta_queue -> prev, delta_entry, list);

    //decrement its delta time
    //printk("DECREMENTING ROOT FROM %d\n", root->delta_time);
    root -> delta_time -= 1;
    //printk("DONE DECREMENTING ROOT: AT %d\n", root->delta_time);

    //remove it if its waiting is done, and any subsequent processes that were waiting for the same time
    for (root; root->delta_time <= 0 && &root->list != delta_queue; root = list_entry(root->list.prev, delta_entry, list))
    {
	//printk("KILLING...\n");
	kthread_stop(root->task);
	wake_up_process(root->task);
	//printk("DEQUEUING...\n");
    	delta_queue -> prev = delta_queue -> prev -> prev;
	delta_queue -> prev -> next = delta_queue;
    }
}

//return how many entries are in the delta queue
int dq_size(struct list_head *delta_queue)
{
	struct list_head *pos;
	int size = 0;
	
	list_for_each(pos, delta_queue)
	{
		size++;
	}

	return size;
}

//a function to put the created processes created to sleep for an indefinite amount of time
int nap(void* data)
{
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule();
	return 0;
}

//function to run kthread_run with the needed parameters
struct task_struct* make_thread(int naptime)
{
	void* naptimeptr = &naptime;
	return kthread_run(nap, naptimeptr, "delta_entry for an initial %d ms", naptime);
}

//method to actually make and run the delta queue
asmlinkage long sys_delta(void)
{
	//an array of 100 randomly generatred numbers in the range of 1-1000. Used as the different times each thread will sleep for in this experiment
	int sleep_times[] = {6503, 4363, 9529, 5259, 3730, 1428, 9558, 7083, 4226, 9760, 3778, 2244, 5511, 5194, 3769, 6417, 745, 7777, 7072, 795, 4951, 558, 3834, 9643, 2668, 515, 8776, 4654, 2707, 570, 7729, 4395, 6742, 6911, 9908, 5395, 1381, 9220, 9162, 1783, 82, 3484, 5781, 4605, 884, 6366, 1149, 2970, 5000, 1285, 7987, 9786, 5607, 8714, 2642, 9048, 8883, 4872, 1545, 5425, 7418, 8389, 2709, 2602, 2869, 5262, 2939, 9808, 7402, 919, 334, 4308, 6390, 9354, 5306, 7809, 2240, 7090, 6001, 1844, 8200, 7613, 9668, 5191, 3381, 996, 4721, 3641, 4334, 1271, 2971, 2334, 9872, 8185, 3626, 7530, 1954, 8026, 845, 1693};
	//the head of the delta_queue
	delta_entry head = {NULL, LIST_HEAD_INIT(head.list), NULL};
	//an array of 100 delta entriek(s for our delta queue
	delta_entry entries[100];
	//counter variable
	int i;
	
	//make delta_entries and enqueue them
	for (i = 0; i < 100; i++)
	{
		delta_entry de = {make_thread(sleep_times[i]), LIST_HEAD_INIT(de.list), sleep_times[i]};
		entries[i] = de;
		dq_add(&entries[i], entries[i].delta_time, &head);
	}

	//main loop
	do
	{
		mdelay(1);
		dq_update(&head.list);
		//printk("just waiting again...")
	}
	while (dq_size(&head.list) > 0);

	return 0;
	
}
