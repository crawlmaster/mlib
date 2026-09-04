#include <mlib/mlib_sll.h>
#include <stdio.h>
#include <stdlib.h>

char *random_chars(size_t length, FILE *source)
{
	char *random = malloc(length + 1);
	if (!random)
		return NULL;
	random[length] = '\0';
	fread(random, 1, length, source);
	for (size_t i = 0; i < length; ++i) {
		random[i] &= 31;
		random[i] += 'A';
	}
	return random;
}

void callback_print(void *data, void *user_data)
{
	(void)user_data;
	printf("%s\n", *(char **)data);
}

int main(void)
{
	FILE *rfp = fopen("/dev/urandom", "rb");
	if (!rfp)
		return 1;
	mlib_sll_t *list = mlib_sll_create(free);
	if (!list)
		return 1;
	for (int i = 0; i < 5; ++i) {
		char *rand_chars = random_chars(4, rfp);
		printf("inserting %s\n", rand_chars);
		(void)mlib_sll_insert_at(list, 1, rand_chars);
	}

	mlib_sll_foreach(list, callback_print, NULL);

	mlib_sll_destroy(&list);
	fclose(rfp);
	return 0;
}
