/**********************************************************************
 gtthread_mutex.c.

 This file contains the implementation of the mutex subset of the
 gtthreads library.  The locks can be implemented with a simple queue.
 **********************************************************************/

/*
 Include as needed
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "gtthread.h"

/*
 The gtthread_mutex_init() function is analogous to
 pthread_mutex_init with the default parameters enforced.
 There is no need to create a static initializer analogous to
 PTHREAD_MUTEX_INITIALIZER.
 */
int gtthread_mutex_init(gtthread_mutex_t* mutex) {
	int *new_mutex_in_heap = malloc(sizeof(int));
//	printf("The malloced address %p\n",new_mutex_in_heap);
	*mutex = new_mutex_in_heap;
	//printf("The malloced address %p\n",*mutex);
	**mutex = 1;

	return 0;
}

/*
 The gtthread_mutex_lock() is analogous to pthread_mutex_lock.
 Returns zero on success.
 */
int gtthread_mutex_lock(gtthread_mutex_t* mutex) {
	// blocking the signal for thread_queue operation
	//printf("Blocking signal!\n");
	sigset_t current, previous;
	sigemptyset(&current);
	sigaddset(&current, SIGPROF);   // Block the scheduler_timer
	sigprocmask(SIG_BLOCK, &current, &previous); //block ing the signal,and store the old setting

	if (**mutex == 1) {
		**mutex = 0;
		// unblock the SIGPROF signal
		//printf("UNBlocking signal!\n");
		sigprocmask(SIG_SETMASK, &previous, NULL );
	} else {
		sigprocmask(SIG_SETMASK, &previous, NULL );
		gtthread_yield();

		int not_ready = 1;
		while (not_ready == 1) {
			printf("##########tried to lock #################\n");
			sigset_t current, previous;
			sigemptyset(&current);
			sigaddset(&current, SIGPROF);   // Block the scheduler_timer
			sigprocmask(SIG_BLOCK, &current, &previous); //block ing the signal,and store the old setting
			if (**mutex == 1) {
				// unblock the SIGPROF signal
				//printf("UNBlocking signal!\n");
				sigprocmask(SIG_SETMASK, &previous, NULL );
				break;
			} else {
				sigprocmask(SIG_SETMASK, &previous, NULL );
				gtthread_yield();

			}

		} //while

	}

	return 0;
}

/*
 The gtthread_mutex_unlock() is analogous to pthread_mutex_unlock.
 Returns zero on success.
 */
int gtthread_mutex_unlock(gtthread_mutex_t *mutex) {
	sigset_t current, previous;
	sigemptyset(&current);
	sigaddset(&current, SIGPROF);   // Block the scheduler_timer
	sigprocmask(SIG_BLOCK, &current, &previous); //block ing the signal,and store the old setting
	**mutex = 1;
	sigprocmask(SIG_SETMASK, &previous, NULL );

	return 0;
}

/*
 The gtthread_mutex_destroy() function is analogous to
 pthread_mutex_destroy and frees any resourcs associated with the mutex.
 */
int gtthread_mutex_destroy(gtthread_mutex_t *mutex) {
	//printf("The address to be freed %p\n",*mutex);
	//fflush(stdout);
	if (*mutex != NULL ) {
		//printf("address %p %p",mutex,*mutex);
		//fflush(stdout);
		free(*mutex);
		*mutex = NULL;
	}
	return 0;
}
