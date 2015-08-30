/**********************************************************************
 gtthread_sched.c.

 This file contains the implementation of the scheduling subset of the
 gtthreads library.  A simple round-robin queue should be used.
 **********************************************************************/
/*
 Include as needed
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <memory.h>
#include <stdlib.h>

#include "gtthread.h"

#define GTTHREAD_STACK_SIZE 1024*64
/*
 Students should define global variables and helper functions as
 they see fit.
 */
steque_t g_thread_queue;
steque_t g_thread_finished_queue;
gtthread_t g_main_thread;

void *gtthread_func_wrapper(void *(*start_routine)(void *), void *arg);

/*
 The gtthread_init() function does not have a corresponding pthread equivalent.
 It must be called from the main thread before any other GTThreads
 functions are called. It allows the caller to specify the scheduling
 period (quantum in micro second), and may also perform any other
 necessary initialization.  If period is zero, then thread switching should
 occur only on calls to gtthread_yield().

 Recall that the initial thread of the program (i.e. the one running
 main() ) is a thread like any other. It should have a
 gtthread_t that clients can retrieve by calling gtthread_self()
 from the initial thread, and they should be able to specify it as an
 argument to other GTThreads functions. The only difference in the
 initial thread is how it behaves when it executes a return
 instruction. You can find details on this difference in the man page
 for pthread_create.
 */
void gtthread_scheduler() {
// blocking the signal for thread_queue operation
//printf("scheduler get called !:  ");
	fflush(stdout);
//printf("Blocking signal!\n");
	sigset_t current, previous;
	sigemptyset(&current);
	sigaddset(&current, SIGPROF);   // Block the scheduler_timer
	sigprocmask(SIG_BLOCK, &current, &previous); //block ing the signal,and store the old setting
// check return condition

	gtthread_t *current_thread_ptr = (gtthread_t *) steque_front(&g_thread_queue);
	int size = steque_size(&g_thread_queue);
	if (size == 1) {
		// unblock the SIGPROF signalgtthread_exit
		//printf("UNBlocking signal!\n");
		sigprocmask(SIG_SETMASK, &previous, NULL );
		//	printf("I am done!Returning to the main thread!!! \n");

		//  swap the context
		return;
		//setcontext(&current_thread_ptr->context);

	}

	if (current_thread_ptr->finished == 0)

	{
		steque_cycle(&g_thread_queue);  //move the front to back

		gtthread_t *next_thread_ptr = (gtthread_t *) steque_front(&g_thread_queue);

// unblock the SIGPROF signalgtthread_exit
		//printf("UNBlocking signal!\n");
		sigprocmask(SIG_SETMASK, &previous, NULL );
//	printf("Switching from thread %d to %d  thread! \n", current_thread_ptr->id,next_thread_ptr->id);

//  swap the context
		swapcontext(&current_thread_ptr->context, &next_thread_ptr->context);
	} else if (current_thread_ptr->finished == 1) {
		current_thread_ptr=steque_pop(&g_thread_queue);  //Remove the finished thread from the queue
		// free the resources
		free(current_thread_ptr->context.uc_stack.ss_sp);
		free(current_thread_ptr);

		gtthread_t *next_thread_ptr = (gtthread_t *) steque_front(&g_thread_queue);

		// unblock the SIGPROF signalgtthread_exit
		//printf("UNBlocking signal!\n");
		sigprocmask(SIG_SETMASK, &previous, NULL );
//		printf("Switching from finished thread %d to %d  thread! \n", current_thread_ptr->id,next_thread_ptr->id);

		//  swap the context
		setcontext(&next_thread_ptr->context);

	}

}

void gtthread_init(long period) {
	// The unit of measure fo the preiod is us

// set up the thread queues
	steque_init(&g_thread_queue);
	steque_init(&g_thread_finished_queue);
// get the current context and add to the queue;

	gtthread_t* main_thread_ptr = &g_main_thread;
	getcontext(&main_thread_ptr->context);
	main_thread_ptr->id = 0;
	main_thread_ptr->finished = 0;

// push the main thread to the back of the queue
	steque_enqueue(&g_thread_queue, (steque_item) main_thread_ptr);
//  if period is larger than 0, set up timer for scheduling
// Otherwise, the scheduler will be called direclty
	if (period > 0) {
// setup the timer for scheduler
		struct sigaction sa;
		struct itimerval scheduler_timer;
		/* Install timer_handler as the signal handler for SIGVTALRM. */
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = &gtthread_scheduler;
// unblock the signal
		//  sigemptyset(&sa.sa_mask);
		//  sa.sa_flags = 0;

		sigaction(SIGPROF, &sa, NULL );

		/* Configure the timer to expire after the period  */
		scheduler_timer.it_value.tv_sec = 0;
		scheduler_timer.it_value.tv_usec = period;
		/* ... and every 250 msec after that. */
		scheduler_timer.it_interval.tv_sec = 0;
		scheduler_timer.it_interval.tv_usec = period;
		/* Start a virtual timer. It counts down whenever this process is
		 executing. */
		setitimer(ITIMER_PROF, &scheduler_timer, NULL );
	}
}
void *gtthread_func_wrapper(void *(*start_routine)(void *), void *arg) {
	void *return_value;
	return_value = (*start_routine)(arg);
	gtthread_exit(return_value);
}

/*
 The gtthread_create() function mirrors the pthread_create() function,
 only default attributes are always assumed.
 */
int gtthread_create(gtthread_t *thread, void *(*start_routine)(void *), void *arg) {
// creat a new thread
//
	gtthread_t *new_thread_ptr = (gtthread_t *) malloc(sizeof(gtthread_t));
	// eorro handling
	if (new_thread_ptr == NULL) {
	    fprintf(stderr, "failed to allocate memory.\n");
	    return -1;
	}

//
	getcontext(&new_thread_ptr->context);

	new_thread_ptr->context.uc_link = &g_main_thread.context;
	new_thread_ptr->context.uc_stack.ss_sp = malloc( GTTHREAD_STACK_SIZE);
	new_thread_ptr->context.uc_stack.ss_size = GTTHREAD_STACK_SIZE;
	new_thread_ptr->context.uc_stack.ss_flags = 0;
	// error handling
	if (new_thread_ptr->context.uc_stack.ss_sp == NULL) {
	    fprintf(stderr, "failed to allocate memory.\n");
	    return -1;
	}

	// set new id number
	new_thread_ptr->id = steque_size(&g_thread_queue);
	new_thread_ptr->finished = 0;

	makecontext(&new_thread_ptr->context, gtthread_func_wrapper, 2, start_routine, arg);

// Add the thread to the thread_queue

// blocking the signal for thread_queue operation
	//printf("Blocking signal!\n");
	sigset_t current, previous;
	sigemptyset(&current);
	sigaddset(&current, SIGPROF);   // Block the scheduler_timer
	sigprocmask(SIG_BLOCK, &current, &previous); //block ing the signal,and store the old setting

// add the new thread to the thread_queue
	steque_enqueue(&g_thread_queue, (steque_item) new_thread_ptr);

// unblock the SIGPROF signal
	//printf("UNBlocking signal!\n");
	sigprocmask(SIG_SETMASK, &previous, NULL );
	*thread = *new_thread_ptr;

	return 0;
}

/*
 The gtthread_join() function is analogous to pthread_join.
 All gtthreads are joinable.gtthread_create
 */
int gtthread_join(gtthread_t thread, void **status) {
	//   printf("% The thread to be joined %d \n",thread.id);
	int num_elements = steque_size(&g_thread_finished_queue);
	int is_jointed = 0;

	while (num_elements < 1) {
		//   	fprintf(stderr, "No thread finished yet.\n");
		gtthread_yield();
		num_elements = steque_size(&g_thread_finished_queue);

	}
	// trying to find the thread in the finished queue
	// blocking the signal for thread_queue operationgtthread_create
	while (is_jointed == 0) {   //printf("Blocking signal!\n");
		sigset_t current, previous;
		sigemptyset(&current);
		sigaddset(&current, SIGPROF);   // Block the scheduler_timer
		sigprocmask(SIG_BLOCK, &current, &previous); //block ing the signal,and store the old setting
		num_elements = steque_size(&g_thread_finished_queue);
		int i;
		for (i = 0; i < num_elements; i++) {
			gtthread_finished_t *current_thread_ptr = (gtthread_finished_t *) steque_front(&g_thread_finished_queue); // look at the thread from the queue
			//             printf("% The thread to be compared with %d\n",thread.id);
			if (current_thread_ptr->id == thread.id) {

				if (current_thread_ptr->ret != NULL ) {
					*status = current_thread_ptr->ret;
				} else {
					status = NULL;
				}
				is_jointed = 1;
				current_thread_ptr=(gtthread_finished_t *)steque_pop(&g_thread_finished_queue);  // remove the thread from the queue
				// TODO freee the resource!!
				free(current_thread_ptr);
				break;

			}
			steque_cycle(&g_thread_finished_queue);
		}

// unblock the SIGPROF signal
		//printf("UNBlocking signal!\n");
		sigprocmask(SIG_SETMASK, &previous, NULL );
		gtthread_yield();

	}

	return 0;

}

/*
 The gtthread_exit() function is analogous to pthread_exit.
 */
void gtthread_exit(void* retval) {

// blocking the signal for thread_queue operation
	//printf("Blocking signal!\n");
	sigset_t current, previous;
	sigemptyset(&current);
	sigaddset(&current, SIGPROF);   // Block the scheduler_timer
	sigprocmask(SIG_BLOCK, &current, &previous); //block ing the signal,and store the old setting

	gtthread_t *current_thread_ptr = (gtthread_t *) steque_front(&g_thread_queue);  // read the thread from the queue

// add the thread to the finished queue
	gtthread_finished_t *finished_thread_ptr = (gtthread_finished_t*) malloc(sizeof(gtthread_finished_t));

	// error handling
	if (finished_thread_ptr == NULL) {
	    fprintf(stderr, "failed to allocate memory. Program Abort!\n");
	    exit(-1);
	}

	finished_thread_ptr->id = current_thread_ptr->id;
	finished_thread_ptr->ret = retval;

	steque_enqueue(&g_thread_finished_queue, (steque_item) finished_thread_ptr);

// update the ready queue to complete
	current_thread_ptr->finished = 1;
// unblock the SIGPROF signal
	//printf("UNBlocking signal!\n");
	sigprocmask(SIG_SETMASK, &previous, NULL );

// yield
	gtthread_yield();

}

/*
 The gtthread_yield() function is analogous to pthread_yield, causing
 the calling thread to relinquish the cpu and place itself at the
 back of the schedule queue.sigProcMask
 */
void gtthread_yield(void) {

	gtthread_scheduler();

}

/*
 The gtthread_yield() function is analogous to pthread_equal,
 returning zero if the threads are the same and non-zero otherwise.
 */
int gtthread_equal(gtthread_t t1, gtthread_t t2) {

	if (t1.id == t2.id) {
		return 0;
	} else {
		return 1;
	}

}

/*
 The gtthread_cancel() function is analogous to pthread_cancel,
 allowing one thread to terminate another asynchronously.
 */
int gtthread_cancel(gtthread_t thread) {
	printf("Trying to cancel thread %d", thread.id);
	// blocking the signal for thread_queue operation
	//printf("Blocking signal!\n");
	sigset_t current, previous;
	sigemptyset(&current);
	sigaddset(&current, SIGPROF);   // Block the scheduler_timer
	sigprocmask(SIG_BLOCK, &current, &previous); //block ing the signal,and store the old setting
	// go through the queue, tying to find the thread
	int num_elements = steque_size(&g_thread_queue);

	int i;
	for (i = 0; i < num_elements; i++) {
		gtthread_t *current_thread_ptr = (gtthread_t *) steque_front(&g_thread_queue); // read the thread from the queue

		if (current_thread_ptr->id == thread.id) {
			// add the thread to the finished queue
			gtthread_finished_t *finished_thread_ptr = (gtthread_finished_t*) malloc(sizeof(gtthread_finished_t));

			// error handling
			if (finished_thread_ptr == NULL) {
			    fprintf(stderr, "failed to allocate memory!\n");
			    return -1;
			}

			finished_thread_ptr->id = current_thread_ptr->id;
			finished_thread_ptr->ret = NULL;
			steque_enqueue(&g_thread_finished_queue, (steque_item) finished_thread_ptr);

			// Mark the thread finished
			current_thread_ptr->finished = 1;

		}
		steque_cycle(&g_thread_queue);
	}

	// unblock the SIGPROF signal
	//printf("UNBlocking signal!\n");
	sigprocmask(SIG_SETMASK, &previous, NULL );

	// yield
	gtthread_yield();

}

/*
 Returns calling thread.
 */
gtthread_t gtthread_self(void) {

	gtthread_t *thread_ptr = (gtthread_t*) steque_front(&g_thread_queue);
	return *thread_ptr;

}
