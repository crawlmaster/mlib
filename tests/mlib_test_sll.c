#include <mlib/mlib_sll.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define TEST_FAILED "\033[31mFAILED\033[0m"
#define TEST_PASSED "\033[32mPASSED\033[0m"
#define STR_PAD	    -50

#define TEST_ITEMS_COUNT 10

static int sll_free_call_count = 0;

static void create_test_free_fn(void *elem)
{
	(void)elem;
	++sll_free_call_count;
}

static int cmp_int(const void *a, const void *b)
{
	int va = *(const int *)a;
	int vb = *(const int *)b;
	return (va > vb) - (va < vb);
}

static void foreach_increment_cb(void **data, void *user_data)
{
	if (!data || !*data)
		return;

	int *val = (int *)*data;
	int  step = user_data ? *(int *)user_data : 1;
	*val += step;
}

static mlib_sll_t *create_test_list(int *passed_tests, int *total_tests,
				    mlib_free_fn fn)
{
	mlib_sll_t *list = mlib_sll_create(fn);
	if (!list) {
		printf("[%s]\n", TEST_FAILED);
		puts("Not enough memory available to run the test");
		printf("PASSED: %d\tTOTAL: %d\n", *passed_tests, *total_tests);
		return NULL;
	}
	return list;
}

static void print_test_result(bool passed, int *passed_tests, int *total_tests)
{
	++(*total_tests);
	if (passed) {
		printf("[%s]\n", TEST_PASSED);
		++(*passed_tests);
	} else {
		printf("[%s]\n", TEST_FAILED);
	}
}

void test_create(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Create:");

	printf("%*s", STR_PAD, "Creating list");
	mlib_sll_t *list = create_test_list(passed_tests, total_tests,
					    create_test_free_fn);
	if (!list)
		return;

	print_test_result(true, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing initial size == 0");
	print_test_result(mlib_sll_size(list) == 0, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing iterator on empty list");
	mlib_sll_iter_t *it = mlib_sll_head(list);
	bool		 it_ok = (it != NULL) && !mlib_sll_has_next(it);
	mlib_sll_iter_destroy(&it);
	print_test_result(it_ok, passed_tests, total_tests);

	mlib_sll_destroy(&list);
}

void test_insert_head(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Insert Head:");

	printf("%*s", STR_PAD, "Testing insert_head on NULL list");
	int dummy = 1;
	print_test_result(mlib_sll_insert_head(NULL, &dummy) ==
				  MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	mlib_sll_t *list = create_test_list(passed_tests, total_tests, NULL);
	if (!list)
		return;

	int vals[TEST_ITEMS_COUNT];
	for (int i = 0; i < TEST_ITEMS_COUNT; ++i)
		vals[i] = i;

	bool insert_ok = true;
	for (int i = 0; i < TEST_ITEMS_COUNT; ++i) {
		if (mlib_sll_insert_head(list, &vals[i]) != MLIB_SUCCESS) {
			insert_ok = false;
			break;
		}
	}
	printf("%*s", STR_PAD, "Testing insert_head return status");
	print_test_result(insert_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing size after insert_head");
	print_test_result(mlib_sll_size(list) == TEST_ITEMS_COUNT, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing LIFO order");
	mlib_sll_iter_t *it = mlib_sll_head(list);
	bool		 order_ok = true;
	for (int i = TEST_ITEMS_COUNT - 1; i >= 0; --i) {
		if (!mlib_sll_has_next(it)) {
			order_ok = false;
			break;
		}
		int *val = (int *)mlib_sll_iter_data(it);
		if (!val || *val != i) {
			order_ok = false;
			break;
		}
		mlib_sll_iter_next(it);
	}
	mlib_sll_iter_destroy(&it);
	print_test_result(order_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing tail points to first inserted");
	mlib_sll_iter_t *it_end = mlib_sll_tail(list);
	int *tail_val = it_end ? (int *)mlib_sll_iter_data(it_end) : NULL;
	bool tail_ok = (tail_val != NULL) && (*tail_val == 0);
	mlib_sll_iter_destroy(&it_end);
	print_test_result(tail_ok, passed_tests, total_tests);

	mlib_sll_destroy(&list);
}

void test_insert_tail(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Insert Tail:");

	printf("%*s", STR_PAD, "Testing insert_tail on NULL list");
	int dummy = 1;
	print_test_result(mlib_sll_insert_tail(NULL, &dummy) ==
				  MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	mlib_sll_t *list = create_test_list(passed_tests, total_tests, NULL);
	if (!list)
		return;

	int vals[TEST_ITEMS_COUNT];
	for (int i = 0; i < TEST_ITEMS_COUNT; ++i)
		vals[i] = i * 10;

	bool insert_ok = true;
	for (int i = 0; i < TEST_ITEMS_COUNT; ++i) {
		if (mlib_sll_insert_tail(list, &vals[i]) != MLIB_SUCCESS) {
			insert_ok = false;
			break;
		}
	}
	printf("%*s", STR_PAD, "Testing insert_tail return status");
	print_test_result(insert_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing size after insert_tail");
	print_test_result(mlib_sll_size(list) == TEST_ITEMS_COUNT, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing FIFO order");
	mlib_sll_iter_t *it = mlib_sll_head(list);

	bool order_ok = true;
	for (int i = 0; i < TEST_ITEMS_COUNT; ++i) {
		if (!mlib_sll_has_next(it)) {
			order_ok = false;
			break;
		}
		int *val = (int *)mlib_sll_iter_data(it);
		if (!val || *val != i * 10) {
			order_ok = false;
			break;
		}
		mlib_sll_iter_next(it);
	}
	mlib_sll_iter_destroy(&it);
	print_test_result(order_ok, passed_tests, total_tests);

	mlib_sll_destroy(&list);
}

void test_insert_at(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Insert At:");

	mlib_sll_t *list = create_test_list(passed_tests, total_tests, NULL);
	if (!list)
		return;

	int val_head = 100;
	printf("%*s", STR_PAD, "Testing insert_at index 0 on empty list");
	print_test_result(mlib_sll_insert_at(list, 0, &val_head) ==
				  MLIB_SUCCESS,
			  passed_tests, total_tests);

	int val_tail = 300;
	printf("%*s", STR_PAD, "Testing insert_at out of bounds");
	print_test_result(mlib_sll_insert_at(list, 99, &val_tail) ==
				  MLIB_SUCCESS,
			  passed_tests, total_tests);

	int val_mid = 200;
	printf("%*s", STR_PAD, "Testing insert_at middle index");
	print_test_result(mlib_sll_insert_at(list, 1, &val_mid) == MLIB_SUCCESS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing order after insert_at operations");
	int		 expected[] = { 100, 200, 300 };
	mlib_sll_iter_t *it = mlib_sll_head(list);
	bool		 order_ok = true;
	for (int i = 0; i < 3; ++i) {
		int *val = (int *)mlib_sll_iter_data(it);
		if (!val || *val != expected[i]) {
			order_ok = false;
			break;
		}
		mlib_sll_iter_next(it);
	}
	mlib_sll_iter_destroy(&it);
	print_test_result(order_ok, passed_tests, total_tests);

	mlib_sll_destroy(&list);
}

void test_remove_head(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Remove Head:");

	mlib_sll_t *list = create_test_list(passed_tests, total_tests, NULL);
	if (!list)
		return;

	printf("%*s", STR_PAD, "Testing remove_head on empty list");
	print_test_result(mlib_sll_remove_head(list) == MLIB_ERR_EMPTY,
			  passed_tests, total_tests);

	int v1 = 10;
	int v2 = 20;
	mlib_sll_insert_tail(list, &v1);
	mlib_sll_insert_tail(list, &v2);

	printf("%*s", STR_PAD, "Testing remove_head on multi-element list");
	print_test_result(mlib_sll_remove_head(list) == MLIB_SUCCESS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing remaining element after remove_head");
	mlib_sll_iter_t *it = mlib_sll_head(list);
	int		*head_val = it ? (int *)mlib_sll_iter_data(it) : NULL;
	bool		 head_ok = (head_val != NULL) && (*head_val == 20);
	mlib_sll_iter_destroy(&it);
	print_test_result(head_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing remove_head down to empty list");
	print_test_result(mlib_sll_remove_head(list) == MLIB_SUCCESS &&
				  mlib_sll_size(list) == 0,
			  passed_tests, total_tests);

	mlib_sll_destroy(&list);
}

void test_remove_tail(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Remove Tail:");

	mlib_sll_t *list = create_test_list(passed_tests, total_tests, NULL);
	if (!list)
		return;

	printf("%*s", STR_PAD, "Testing remove_tail on empty list");
	print_test_result(mlib_sll_remove_tail(list) == MLIB_ERR_EMPTY,
			  passed_tests, total_tests);

	int v1 = 10;
	int v2 = 20;
	mlib_sll_insert_tail(list, &v1);
	mlib_sll_insert_tail(list, &v2);

	printf("%*s", STR_PAD, "Testing remove_tail on multi-element list");
	print_test_result(mlib_sll_remove_tail(list) == MLIB_SUCCESS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing tail after remove_tail");
	mlib_sll_iter_t *it = mlib_sll_tail(list);
	int		*tail_val = it ? (int *)mlib_sll_iter_data(it) : NULL;
	bool		 tail_ok = (tail_val != NULL) && (*tail_val == 10);
	mlib_sll_iter_destroy(&it);
	print_test_result(tail_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing remove_tail on single-element list");
	print_test_result(mlib_sll_remove_tail(list) == MLIB_SUCCESS &&
				  mlib_sll_size(list) == 0,
			  passed_tests, total_tests);

	mlib_sll_destroy(&list);
}

void test_remove_at(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Remove At:");

	mlib_sll_t *list = create_test_list(passed_tests, total_tests, NULL);
	if (!list)
		return;

	int vals[5] = { 0, 10, 20, 30, 40 };
	for (int i = 0; i < 5; ++i)
		mlib_sll_insert_tail(list, &vals[i]);

	printf("%*s", STR_PAD, "Testing remove_at out of bounds");
	print_test_result(mlib_sll_remove_at(list, 10) ==
				  MLIB_ERR_OUT_OF_BOUNDS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing remove_at middle index (index 2)");
	print_test_result(mlib_sll_remove_at(list, 2) == MLIB_SUCCESS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing elements order after remove_at");
	int		 expected[] = { 0, 10, 30, 40 };
	mlib_sll_iter_t *it = mlib_sll_head(list);
	bool		 order_ok = true;
	for (int i = 0; i < 4; ++i) {
		int *val = (int *)mlib_sll_iter_data(it);
		if (!val || *val != expected[i]) {
			order_ok = false;
			break;
		}
		mlib_sll_iter_next(it);
	}
	mlib_sll_iter_destroy(&it);
	print_test_result(order_ok, passed_tests, total_tests);

	mlib_sll_destroy(&list);
}

void test_find(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Find:");

	mlib_sll_t *list = create_test_list(passed_tests, total_tests, NULL);
	if (!list)
		return;

	int vals[5] = { 11, 22, 33, 44, 55 };
	for (int i = 0; i < 5; ++i)
		mlib_sll_insert_tail(list, &vals[i]);

	size_t idx = 0;
	int    target = 33;
	printf("%*s", STR_PAD, "Testing find existing element");
	bool find_ok =
		(mlib_sll_find(list, &target, cmp_int, &idx) == MLIB_SUCCESS) &&
		(idx == 2);
	print_test_result(find_ok, passed_tests, total_tests);

	int missing = 999;
	printf("%*s", STR_PAD, "Testing find missing element");
	print_test_result(mlib_sll_find(list, &missing, cmp_int, &idx) ==
				  MLIB_ERR_NOT_FOUND,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing find on NULL list or callback");
	print_test_result(mlib_sll_find(NULL, &target, cmp_int, &idx) ==
					  MLIB_ERR_NULL_PTR &&
				  mlib_sll_find(list, &target, NULL, &idx) ==
					  MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	mlib_sll_destroy(&list);
}

void test_foreach(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Foreach:");

	mlib_sll_t *list = create_test_list(passed_tests, total_tests, NULL);
	if (!list)
		return;

	int vals[5] = { 0, 1, 2, 3, 4 };
	for (int i = 0; i < 5; ++i)
		mlib_sll_insert_tail(list, &vals[i]);

	int step = 5;
	printf("%*s", STR_PAD, "Testing foreach modification callback");
	print_test_result(mlib_sll_foreach(list, foreach_increment_cb, &step) ==
				  MLIB_SUCCESS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing values updated by foreach");
	mlib_sll_iter_t *it = mlib_sll_head(list);
	bool		 updated_ok = true;
	for (int i = 0; i < 5; ++i) {
		int *val = (int *)mlib_sll_iter_data(it);
		if (!val || *val != i + step) {
			updated_ok = false;
			break;
		}
		mlib_sll_iter_next(it);
	}
	mlib_sll_iter_destroy(&it);
	print_test_result(updated_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing foreach NULL arguments");
	print_test_result(mlib_sll_foreach(NULL, foreach_increment_cb, NULL) ==
					  MLIB_ERR_NULL_PTR &&
				  mlib_sll_foreach(list, NULL, NULL) ==
					  MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	mlib_sll_destroy(&list);
}

void test_iterators(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Iterators:");

	mlib_sll_t *list = create_test_list(passed_tests, total_tests, NULL);
	if (!list)
		return;

	int vals[4] = { 0, 100, 200, 300 };
	for (int i = 0; i < 4; ++i)
		mlib_sll_insert_tail(list, &vals[i]);

	printf("%*s", STR_PAD, "Testing mlib_sll_at valid index (index 2)");
	mlib_sll_iter_t *it_at = mlib_sll_at(list, 2);
	int *val_at = it_at ? (int *)mlib_sll_iter_data(it_at) : NULL;
	bool at_ok = (val_at != NULL) && (*val_at == 200);
	mlib_sll_iter_destroy(&it_at);
	print_test_result(at_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing iter_data and has_next on NULL iter");
	print_test_result(mlib_sll_iter_data(NULL) == NULL &&
				  !mlib_sll_has_next(NULL),
			  passed_tests, total_tests);

	mlib_sll_destroy(&list);
}

void test_clear_and_destroy(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB SLL Clear and Destroy:");

	sll_free_call_count = 0;
	mlib_sll_t *list = create_test_list(passed_tests, total_tests,
					    create_test_free_fn);
	if (!list)
		return;

	int dummies[5] = { 1, 2, 3, 4, 5 };
	for (int i = 0; i < 5; ++i)
		mlib_sll_insert_tail(list, &dummies[i]);

	printf("%*s", STR_PAD, "Testing clear resets size to 0");
	mlib_sll_clear(list);
	print_test_result(mlib_sll_size(list) == 0, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing free_fn invocation count on clear");
	print_test_result(sll_free_call_count == 5, passed_tests, total_tests);

	/* Reinseram elemente pentru a testa destroy pe lista ne-goala */
	for (int i = 0; i < 3; ++i)
		mlib_sll_insert_tail(list, &dummies[i]);

	printf("%*s", STR_PAD, "Testing destroy cleans elements and sets NULL");
	mlib_sll_destroy(&list);
	bool destroy_ok = (list == NULL) && (sll_free_call_count == 8);
	print_test_result(destroy_ok, passed_tests, total_tests);
}

int main(void)
{
	int passed_tests = 0, total_tests = 0;

	test_create(&passed_tests, &total_tests);
	test_insert_head(&passed_tests, &total_tests);
	test_insert_tail(&passed_tests, &total_tests);
	test_insert_at(&passed_tests, &total_tests);
	test_remove_head(&passed_tests, &total_tests);
	test_remove_tail(&passed_tests, &total_tests);
	test_remove_at(&passed_tests, &total_tests);
	test_find(&passed_tests, &total_tests);
	test_foreach(&passed_tests, &total_tests);
	test_iterators(&passed_tests, &total_tests);
	test_clear_and_destroy(&passed_tests, &total_tests);

	printf("\n==================================================\n");
	printf("SUMMARY: %d / %d tests passed\n", passed_tests, total_tests);
	printf("==================================================\n");

	return (passed_tests == total_tests) ? 0 : 1;
}
