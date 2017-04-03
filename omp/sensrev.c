#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>

#define BARR_ITER 256.0

enum {FASLE, TRUE};
int nthreads;
int count;
char global_sense = TRUE; /* shared among processors/threads */

void senserev_bar(char *localsense, int nthreads, int thread_num)
{
	extern int count;

	//printf("thread %d: global count = %d\n", thread_num, count);
	//printf("thread %d: local sense: %d\n", thread_num, *localsense);
	*localsense = !*localsense;
	//printf("thread %d: local sense after rev: %d\n", thread_num, *localsense);
#pragma omp atomic
	count--; /* atomic_decrement(count); */
	/* printf("thread %d: global count = %d\n", thread_num, count); */
	if (count == 0) { /* last thread */
		//printf("thread %d switching sense\n", thread_num);
		count = nthreads;
		global_sense = *localsense;
	} else {
		while (global_sense != *localsense)
			; /* spin */
	}
}

int main(int argc, char *argv[])
{
	extern int count;
	extern int nthreads;
	int thread_num;
	int priv;
	int pub;
	struct timeval tv_before;
	struct timeval tv_after;
	double total_time;
	double avg_time;

	nthreads = atoi(*++argv);
	count = nthreads;
	thread_num = -1;
	priv = 0;
	pub = 0;
	total_time = 0;
	
#pragma omp parallel num_threads(nthreads) firstprivate(thread_num, priv) shared(pub, total_time)
	{
		thread_num = omp_get_thread_num();
		char localsense; /* local to each processor */
		localsense = TRUE;
		unsigned long thread_time;
		double thread_avg;

		thread_time = 0;
		thread_avg = 0;
		for (int i = 0; i < BARR_ITER; i++) {
#pragma omp critical
			{
				priv += thread_num;
				pub += thread_num;
			}
			gettimeofday(&tv_before, NULL);
			senserev_bar(&localsense, nthreads, thread_num);
			gettimeofday(&tv_after, NULL);
			thread_time += tv_after.tv_usec - tv_before.tv_usec;
			printf("[iter %d] time spent by thread %d: %lu\n", i, thread_num, thread_time);
		}
		thread_avg = thread_time / BARR_ITER;
		printf("thread %d: avg time = %f\n", thread_num, thread_avg);
		total_time += thread_avg;
	}
	avg_time = total_time / nthreads;
	printf("average time: %f\n",  avg_time);

	return 0;
}
