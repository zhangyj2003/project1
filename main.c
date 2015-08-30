#include <stdio.h>
#include <stdlib.h>
#include "gtthread.h"
#include <unistd.h>

#define CYCLE_NUM 10000

int g_count = 0;
gtthread_mutex_t g_mutex;

/* Tests creation.
 Should print "Hello World!" */
double killCycle(double start, int num) {
	double sum = start;
	int i;
	int j;
	for (i = 0; i < num; i++)
		for (j = 0; j < num; j++) {
			sum = sum + i + j;
		}
	return sum;
}

double* thr1(void *in) {
	printf("Hello World! from thread 1\n");
	fflush(stdout);

	double sum;

	gtthread_mutex_lock(&g_mutex);
	printf("%p,%p\n",g_mutex,&g_mutex);

	g_count = g_count + 1;
	printf("global count from thread 1%d\n", g_count);
	sum = killCycle(1, CYCLE_NUM);
	gtthread_mutex_unlock(&g_mutex);

//  gtthread_exit(NULL);
	printf("returning from thread 1 %f \n", sum);
//  gtthread_exit(NULL);
	double *ret_ptr = malloc(sizeof(double));
	*ret_ptr = sum;
	printf("returning from thread with value= %f \n", sum);
	return ret_ptr;
//  gtthread_yield();

}

void *thr2(void *in) {
	printf("Hello World! from thread 2\n");
	fflush(stdout);
	double sum;

	gtthread_mutex_lock(&g_mutex);
	g_count = g_count + 2;
	printf("global count from thread 2 %d\n", g_count);
	gtthread_mutex_unlock(&g_mutex);

	sum = killCycle(2, CYCLE_NUM);

//  gtthread_exit(NULL);
	printf("returning from thread 2 %f \n", sum);
	return NULL ;
//  gtthread_yield();

}

void *thr3(void *in) {
	printf("Hello World! from thread 3\n");
	fflush(stdout);
	double sum;

	gtthread_mutex_lock(&g_mutex);
	g_count = g_count + 3;
	printf("global count from thread 3 %d\n", g_count);
	gtthread_mutex_unlock(&g_mutex);

	sum = killCycle(3, CYCLE_NUM);
//  gtthread_exit(NULL);
	printf("returning from thread 3 %f \n", sum);
	return NULL ;
//  gtthread_yield();

}

int main() {
	gtthread_t t1;
	gtthread_t t2;
	gtthread_t t3;
	double sum_thread1 = 0;
	double *p1 = &sum_thread1;
	double **p2 = &p1;

	gtthread_mutex_init(&g_mutex);

	gtthread_init(10000);

	gtthread_create(&t1, thr1, NULL );

	gtthread_create(&t2, thr2, NULL );

	gtthread_create(&t3, thr3, NULL );

	printf("id of thread1 is %d\n", t1.id);
	printf("id of thread1 is %d\n", t2.id);
	// get self id
	gtthread_t t0;
	t0 = gtthread_self();
	printf("The main thread is %d \n", t0.id);

	//gtthread_yield();
	//gtthread_cancel(t2);

	gtthread_join(t1, (void **) p2);
	printf("Returned Value from thread 1 = %f\n", **p2);

	gtthread_join(t2, NULL );
	gtthread_join(t3, NULL );



	printf("returning to main thread!\n");

	// printf("returning to main thread again!\n");
	gtthread_mutex_destroy(&g_mutex);

    printf("\n....Exiting the Main!....\n");

	return EXIT_SUCCESS;
}
