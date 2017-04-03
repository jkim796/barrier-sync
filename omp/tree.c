#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <omp.h>
#include "queue.h"

#define log2(x) ((int) (log(x) / log(2)))
#define getlevels(nthreads) log2(nthreads)

#define BARR_ITER 4.0

enum {FALSE, TRUE};

struct tnode *createnode();
struct tnode *createtree(int levels, struct tnode *parent);
struct tnode **getleaves(struct tnode *p, int nleaves);
void treeprint(struct tnode *p);
void printleaves(struct tnode **leaves, int nleaves);
void tree_bar(struct tnode *node, char *localsense, int);
int power2(int p);

struct tnode {
	char locksense; /* 'global' to two threads */
	int count;
	struct tnode *parent;
	struct tnode *left;
	struct tnode *right;
};

struct tnode *createnode()
{
	struct tnode *node;

	node = malloc(sizeof(struct tnode));
	node->locksense = TRUE;
	node->count = 2;
	node->left = node->right = NULL;
	return node;
}

struct tnode *createtree(int levels, struct tnode *parent)
{
	struct tnode *p;

	p = NULL;
	if (levels > 0) {
		p = createnode();
		p->left = createtree(levels - 1, p);
		p->right = createtree(levels - 1, p);
		p->parent = parent;
	}
	return p;
}

struct tnode **getleaves(struct tnode *p, int nleaves)
{
	struct tnode **leaves;
	struct tnode **leaves_cp;
	struct tnode *node;

	leaves = (struct tnode **) malloc(nleaves * sizeof(struct tnode *));
	leaves_cp = leaves;

	/* BFS */
	enqueue(p);
	while (qlen() != 0) {
		node = (struct tnode *) dequeue();
		if (node->left != NULL && node->right != NULL) {
			enqueue(node->left);
			enqueue(node->right);
		} else
			*leaves_cp++ = node;
	}
	return leaves;
}

void treeprint(struct tnode *p)
{
	if (p != NULL) {
		treeprint(p->left);
		printf("%d\t%d\n", p->count, p->locksense);
		treeprint(p->right);
	}
}

void printleaves(struct tnode **leaves, int nleaves)
{
	while (nleaves-- > 0)
		printf("%p\t", *leaves++);
	printf("\n");
}

int power2(int p)
{
	int ans;

	ans = 1;
	while (p-- > 0)
		ans *= 2;
	return ans;
}

void _tree_bar(struct tnode *node, char *localsense, int thread)
{
#pragma omp atomic
	(node->count)--;
	//printf("thread %d: node count: %d\n", thread, node->count);
	//printf("thread %d: node locksense: %d\n", thread, node->locksense);
	if (node->parent == NULL) {
		//printf("thread %d: root reached\n", thread);
		if (node->count == 0) {
			node->count = 2;
			node->locksense = *localsense;
		}
		else
			while (*localsense != node->locksense)
				; /* spin */
	} else {
		if (node->count == 0) {
			_tree_bar(node->parent, localsense, thread);
			node->count = 2;
			node->locksense = *localsense;
		}
		else {
			while (*localsense != node->locksense)
				; /* spin */
		}
	}
}

void tree_bar(struct tnode *node, char *localsense, int thread)
{
	*localsense = !*localsense;
	//printf("thread %d: localsense = %d\tleaf: %p\n", thread, *localsense, node);
	_tree_bar(node, localsense, thread);
}

int main(int argc, char *argv[])
{
	int nthreads, nleaves, levels;
	int thread_num;
	int priv;
	int pub;
	struct tnode *root;
	struct tnode **leaves;
	struct timeval tv_before;
	struct timeval tv_after;
	double total_time;
	double avg_time;

	nthreads = atoi(*++argv);
	levels = getlevels(nthreads);
	nleaves = nthreads / 2;
	root = createtree(levels, NULL);
	leaves = getleaves(root, nleaves);

	thread_num = -1;
	priv = 0;
	pub = 0;
	total_time = 0;

#pragma omp parallel num_threads(nthreads) firstprivate(thread_num, priv) shared(pub)
	{
		thread_num = omp_get_thread_num();
		//printf("thread %d: hello\n", thread_num);

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
			tree_bar(leaves[thread_num/2], &localsense, thread_num);
			gettimeofday(&tv_after, NULL);
			thread_time += tv_after.tv_usec - tv_before.tv_usec;
			printf("[iter %d] time spent by thread %d: %lu\n", i, thread_num, thread_time);
		}
		thread_avg = thread_time / BARR_ITER;
		//printf("thread %d: avg time = %f\n", thread_num, thread_avg);
		total_time += thread_avg;
	}
	avg_time = total_time / nthreads;
	printf("average time: %f\n",  avg_time);
	
	return 0;
}
