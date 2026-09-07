#include <mlib/mlib_vector.h>
#include <stdio.h>

#define INITIAL_N 10

void add_to_sum(void *user_data, void *data)
{
	float x = *(float *)user_data;
	*(float *)data += x;
}

int main(void)
{
	freopen("data.in", "r", stdin);

	mlib_vector_t *v = mlib_vector_create(sizeof(float), INITIAL_N, NULL);
	if (unlikely(!v))
		exit(1);

	int n;
	printf("Enter dataset size: ");
	scanf("%d", &n);
	mlib_vector_resize(v, n);

	puts("Enter data:");
	float x, avg;
	for (int i = 0; i < n; ++i) {
		scanf("%f", &x);
		mlib_vector_push_back(v, &x);
	}

	mlib_vector_foreach(v, add_to_sum, &avg);
	printf("Average: %.2f\n", avg / n);

	return 0;
}
