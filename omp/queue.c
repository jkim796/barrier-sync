#define MAXSIZ 128

static void *queue[MAXSIZ];
static void **head = &queue[0];
static void **tail = &queue[0];

void enqueue(void *data)
{
	if (tail - head < MAXSIZ)
		*tail++ = data;
}

void *dequeue(void)
{
	void *data;

	data = *head++;
	return data;
}	

int qlen(void)
{
	return tail - head;
}
