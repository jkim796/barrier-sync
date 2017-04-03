#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <omp.h>
#include "mpi.h"

#define MSGSIZE 1
#define TAG 1

#define BAR_ITER 32.0

enum role {
	MASTER
};

enum bool {
	FASLE,
	TRUE
};

enum bool globalsense_thread; /* shared among threads */
enum bool globalsense_proc; /* shared among processes */
int nthreads;
int count;
int nprocs;

/**
 * centralized sense reversing barrier for each thread
 */
void senserev_bar(enum bool *localsense_thread, int nthreads, int thread_num)
{
	extern enum bool globalsense_thread;
	extern int count;

	//printf("thread %d: global count = %d\n", thread_num, count);
	//printf("thread %d: local sense: %d\n", thread_num, *localsense_thread);
	*localsense_thread = !*localsense_thread;
	//printf("thread %d: local sense after rev: %d\n", thread_num, *localsense_thread);
#pragma omp atomic
	count--; /* atomic_decrement(count); */
	/* printf("thread %d: global count = %d\n", thread_num, count); */
	if (count == 0) { /* last thread */
		//printf("thread %d switching sense\n", thread_num);
		count = nthreads;
		globalsense_thread = *localsense_thread;
	} else {
		while (globalsense_thread != *localsense_thread)
			; /* spin */
	}
}

/**
 * centralized sense reversing barrier for each process
 */
void centbar(int pid, enum bool *localsense_proc)
{
	extern enum bool globalsense_proc;
	int src;
	MPI_Status status;

	*localsense_proc = !*localsense_proc;
	if (pid == MASTER) {
		/* spin on RECV from all the other procs */
		for (src = 1; src < nprocs; src++)
			MPI_Recv(localsense_proc, MSGSIZE, MPI_INT, src, TAG, MPI_COMM_WORLD, &status);
	} else {
		/* send to MASTER */
		MPI_Send(localsense_proc, MSGSIZE, MPI_INT, MASTER, TAG, MPI_COMM_WORLD);
	}

	//printf("proc %d: globalsense_proc = %d\n", pid, globalsense_proc);
	if (pid == MASTER) {
		globalsense_proc = *localsense_proc;
		//printf("proc %d: globalsense_proc = %d\n", pid, globalsense_proc);
		//printf("proc %d: broadcasting globalsense_proc = %d\n", pid, globalsense_proc);
	}
	//printf("proc %d: before bcast: globalsense_proc = %d\n", pid, globalsense_proc);
	MPI_Bcast(&globalsense_proc, MSGSIZE, MPI_INT, MASTER, MPI_COMM_WORLD);

	/* other procs: spin on broadcast from MASTER */
	while (*localsense_proc != globalsense_proc)
		; /* spin */
	//printf("proc %d: after bcast: globalsense_proc = %d\n", pid, globalsense_proc);
}

int main(int argc, char **argv)
{
	extern enum bool globalsense_thread;
	extern enum bool globalsense_proc;
	extern int nthreads;
	extern int count;
	extern int nprocs;
	enum bool localsense_proc;
	int pid;
	int thread_num;
	int priv;
	int pub;
	unsigned long time_elapsed;
	unsigned long total_time;
	struct timeval tv_before;
	struct timeval tv_after;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &pid);

	globalsense_thread = TRUE;
	globalsense_proc = TRUE;
	localsense_proc = TRUE;
	nthreads = atoi(*++argv);
	count = nthreads;

	thread_num = -1;
	priv = 0;
	pub = 0;

	int i;
	total_time = 0;
	time_elapsed = 0;
	double thread_avg;
	thread_avg = 0;

	for (i = 0; i < BAR_ITER; i++) {
		omp_set_num_threads(nthreads);
#pragma omp parallel firstprivate(thread_num, priv) shared(pub, total_time)
		{
			unsigned long thread_time;
			thread_time = 0;
			
			thread_num = omp_get_thread_num();
			enum bool localsense_thread; /* local to each thread */
			localsense_thread = TRUE;
#pragma omp critical
			{
				priv += thread_num;
				pub += thread_num;
			}
			/* thread barrier */
			//printf("[Before thread barrier] thread %d of proc %d: pub = %d\n", thread_num, pid, pub);
			gettimeofday(&tv_before, NULL);
			senserev_bar(&localsense_thread, nthreads, thread_num);
			gettimeofday(&tv_after, NULL);

			thread_time = tv_after.tv_usec - tv_before.tv_usec;
#pragma omp critical
			total_time += thread_time;
			
			//printf("[After thread barrier] thread %d of proc %d: final value of pub = %d\n ", thread_num, pid, pub);
		} /* implied barrier */
		//printf("[Before Proc Barrier] proc %d: \n", pid);
		thread_avg = total_time / nthreads;

		gettimeofday(&tv_before, NULL);
		centbar(pid, &localsense_proc);
		gettimeofday(&tv_after, NULL);

		time_elapsed += tv_after.tv_usec - tv_before.tv_usec + thread_avg;
		//printf("[After Proc Barrier] proc %d: \n", pid);
	}

	printf("proc %d: avg time = %f\n", pid, time_elapsed / BAR_ITER);

	MPI_Finalize();
	return 0;
}
