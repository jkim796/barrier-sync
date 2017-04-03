#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "mpi.h"

#define MSGSIZE 2
#define log2(x) ((int) (log(x) / log(2)))
#define getrounds(nprocs) log2(nprocs)

#define BAR_ITER 512

int totalrounds;

enum role {
	CHAMPION,
	WINNER,
	LOSER,
};

enum bool {
	FALSE,
	TRUE
};

enum round {
	FIRST = 1
};

int power2(int p)
{
	int ans;

	ans = 1;
	while (p-- > 0)
		ans *= 2;
	return ans;
}

enum bool iswinner(int pid, int round)
{
	if (pid % power2(round) == 0)
		return TRUE;
	else
		return FALSE;
}

enum bool ischamp(int pid)
{
	if (pid == 0)
		return TRUE;
	else
		return FALSE;
}

int findloser(int pid, int round)
{
	int loser;

	loser = pid + power2(round - 1);
	return loser;
}

int findwinner(int pid, int round)
{
	int winner;

	winner = pid - power2(round - 1);
	return winner;
}

void tbar(int pid, int round)
{
	extern int totalrounds;
	int dst, src;
	int msg[MSGSIZE];
	int msg_rcv[MSGSIZE];
	MPI_Status status;

	if (round == totalrounds) { /* final round */
		if (ischamp(pid)) {
			/* spin on recv from loser */
			src = findloser(pid, round);
			MPI_Recv(&msg_rcv, 2, MPI_INT, src, 1, MPI_COMM_WORLD, &status);
			/* wake up loser */
			dst = src;
			MPI_Send(&msg, 2, MPI_INT, dst, 1, MPI_COMM_WORLD);
			return;
		} else {
			/* send to champion */
			dst = findwinner(pid, round);
			MPI_Send(&msg, 2, MPI_INT, dst, 1, MPI_COMM_WORLD);
			/* spin on recv from champ */
			src = dst;
			MPI_Recv(&msg_rcv, 2, MPI_INT, src, 1, MPI_COMM_WORLD, &status);
			return;
		}
	} else {
		if (iswinner(pid, round)) {
			/* spin on recv from loser */
			src = findloser(pid, round);
			MPI_Recv(&msg_rcv, MSGSIZE, MPI_INT, src, 1, MPI_COMM_WORLD, &status);
			/* recurse to next round */
			tbar(pid, round + 1);
			/* wake up loser */
			dst = src;
			MPI_Send(&msg, 2, MPI_INT, dst, 1, MPI_COMM_WORLD);
			return;
		} else {
			/* send to winner */
			dst = findwinner(pid, round);
			MPI_Send(&msg, 2, MPI_INT, dst, 1, MPI_COMM_WORLD);
			/* spin on recv from winner */
			src = dst;
			MPI_Recv(&msg_rcv, 2, MPI_INT, src, 1, MPI_COMM_WORLD, &status);
			return;
		}
	}
}

int main(int argc, char **argv)
{
	extern int totalrounds;
	int nprocs; /* total number of processes */
	int pid; /* this process's rank */

	unsigned long time_elapsed;
	struct timeval tv_before;
	struct timeval tv_after;

	time_elapsed = 0;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &pid);
	
	//printf("proc %d of %d: hello\n", pid, nprocs);
	totalrounds = getrounds(nprocs);

	int i;
	for (i = 0; i < BAR_ITER; i++) {
		//printf("[Before Barrier] proc %d: \n", pid);
		gettimeofday(&tv_before, NULL);
		tbar(pid, FIRST);
		gettimeofday(&tv_after, NULL);
		time_elapsed += tv_after.tv_usec - tv_before.tv_usec;
		printf("proc %d: time elapsed = %lu\n", pid, time_elapsed);
		//printf("[After Barrier] proc %d: \n", pid);
	}

	printf("proc %d: avg time = %lu\n", pid, time_elapsed / BAR_ITER);

	MPI_Finalize();
	return 0;
}
