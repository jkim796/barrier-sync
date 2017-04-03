#include <stdio.h>
#include <sys/time.h>
#include "mpi.h"

#define MSGSIZE 1
#define TAG 1

#define BAR_ITER 512

enum role {
	MASTER
};

enum bool {
	FALSE,
	TRUE
};

enum bool globalsense;
int nprocs;

void centbar(int pid, enum bool *localsense)
{
	extern enum bool globalsense;
	int src;
	MPI_Status status;

	*localsense = !*localsense;
	if (pid == MASTER) {
		/* spin on RECV from all the other procs */
		for (src = 1; src < nprocs; src++)
			MPI_Recv(localsense, MSGSIZE, MPI_INT, src, TAG, MPI_COMM_WORLD, &status);
	} else {
		/* send to MASTER */
		MPI_Send(localsense, MSGSIZE, MPI_INT, MASTER, TAG, MPI_COMM_WORLD);
	}

	//printf("proc %d: globalsense = %d\n", pid, globalsense);
	if (pid == MASTER) {
		globalsense = *localsense;
		//printf("proc %d: globalsense = %d\n", pid, globalsense);
		//printf("proc %d: broadcasting globalsense = %d\n", pid, globalsense);
	}
	//printf("proc %d: before bcast: globalsense = %d\n", pid, globalsense);
	MPI_Bcast(&globalsense, MSGSIZE, MPI_INT, MASTER, MPI_COMM_WORLD);

	/* other procs: spin on broadcast from MASTER */
	while (*localsense != globalsense)
		; /* spin */
	//printf("proc %d: after bcast: globalsense = %d\n", pid, globalsense);
}

int main(int argc, char **argv)
{
	extern enum bool globalsense;
	extern int nprocs;
	int pid;
	enum bool localsense;

	unsigned long time_elapsed;
	struct timeval tv_before;
	struct timeval tv_after;

	localsense = TRUE;
	globalsense = TRUE;
	time_elapsed = 0;
	
	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &pid);

	int i;

	for (i = 0; i < BAR_ITER; i++) {
		//printf("[Before Barrier] proc %d: \n", pid);
		gettimeofday(&tv_before, NULL);
		centbar(pid, &localsense);
		gettimeofday(&tv_after, NULL);
		time_elapsed += tv_after.tv_usec - tv_before.tv_usec;
		printf("proc %d: time elapsed = %lu\n", pid, time_elapsed);
		//printf("[After Barrier] proc %d: \n", pid);
	}

	printf("proc %d: avg time = %lu\n", pid, time_elapsed / BAR_ITER);

	MPI_Finalize();
	return 0;
}
