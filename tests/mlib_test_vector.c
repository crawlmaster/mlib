#include <mlib/common.h>
#include <mlib/mlib_vector.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_FAILED "\033[31mFAILED\033[0m"
#define TEST_PASSED "\033[32mPASSED\033[0m"
#define STR_PAD	    -60

#define TEST_ITEMS_COUNT 10

static int vector_free_call_count = 0;

static char *test_strdup(const char *src)
{
	if (!src)
		return NULL;
	size_t len = strlen(src) + 1;
	char  *dst = malloc(len);
	if (dst)
		memcpy(dst, src, len);
	return dst;
}

static void create_test_free_fn(void *elem)
{
	(void)elem;
	++vector_free_call_count;
}

static int cmp_int(const void *a, const void *b)
{
	int va = *(const int *)a;
	int vb = *(const int *)b;
	return (va > vb) - (va < vb);
}

static void foreach_increment_cb(void *data, void *user_data)
{
	int *val = (int *)data;
	int  step = user_data ? *(int *)user_data : 1;
	*val += step;
}

typedef struct {
	int   id;
	char *tag;
} test_item_t;

typedef struct {
	unsigned char data[300];
} large_struct_t;

static void test_item_cleanup(void *elem)
{
	test_item_t *item = (test_item_t *)elem;
	if (item && item->tag) {
		free(item->tag);
		item->tag = NULL;
	}
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

static mlib_vector_t *create_test_vector(int *passed_tests, int *total_tests,
					 size_t	      elem_size,
					 size_t	      initial_capacity,
					 mlib_free_fn fn)
{
	mlib_vector_t *vect =
		mlib_vector_create(elem_size, initial_capacity, fn);
	if (!vect) {
		print_test_result(false, passed_tests, total_tests);
		puts("Not enough memory available to run the test");
		return NULL;
	}
	return vect;
}

void test_create(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Create:");

	printf("%*s", STR_PAD, "Testing elem_size == 0 fails");
	print_test_result(mlib_vector_create(0, 10, NULL) == NULL, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing capacity overflow on create fails");
	print_test_result(mlib_vector_create(sizeof(int), SIZE_MAX, NULL) ==
				  NULL,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Creating vector with capacity 0");
	mlib_vector_t *vect = create_test_vector(
		passed_tests, total_tests, sizeof(int), 0, create_test_free_fn);
	if (!vect)
		return;

	print_test_result(true, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing initial size == 0");
	print_test_result(mlib_vector_size(vect) == 0, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing initial capacity == 0");
	print_test_result(mlib_vector_capacity(vect) == 0, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing initial data buffer == NULL");
	print_test_result(mlib_vector_data(vect) == NULL, passed_tests,
			  total_tests);

	mlib_vector_destroy(&vect);
}

void test_push_back(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Push Back:");

	printf("%*s", STR_PAD, "Testing push_back on NULL vector");
	int dummy = 1;
	print_test_result(mlib_vector_push_back(NULL, &dummy) ==
				  MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	int  vals[TEST_ITEMS_COUNT];
	bool push_ok = true;
	for (int i = 0; i < TEST_ITEMS_COUNT; ++i) {
		vals[i] = i * 10;
		if (mlib_vector_push_back(vect, &vals[i]) != MLIB_SUCCESS) {
			push_ok = false;
			break;
		}
	}

	printf("%*s", STR_PAD, "Testing push_back return status");
	print_test_result(push_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing size after push_back");
	print_test_result(mlib_vector_size(vect) == TEST_ITEMS_COUNT,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing geometric growth (capacity == 16)");
	print_test_result(mlib_vector_capacity(vect) == 16, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing front and back values");
	int *front_val = (int *)mlib_vector_front(vect);
	int *back_val = (int *)mlib_vector_back(vect);
	bool bounds_ok = (front_val && *front_val == 0) &&
			 (back_val && *back_val == (TEST_ITEMS_COUNT - 1) * 10);
	print_test_result(bounds_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing FIFO order via mlib_vector_get");
	bool order_ok = true;
	for (size_t i = 0; i < TEST_ITEMS_COUNT; ++i) {
		int *val = (int *)mlib_vector_get(vect, i);
		if (!val || *val != (int)i * 10) {
			order_ok = false;
			break;
		}
	}
	print_test_result(order_ok, passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_self_insertion_aliasing(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Self-Insertion Aliasing:");

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 1, NULL);
	if (!vect)
		return;

	int init = 1337;
	mlib_vector_push_back(vect, &init);

	printf("%*s", STR_PAD, "Testing push_back aliased elem forcing growth");
	const int    *elem_in_buffer = (const int *)mlib_vector_get(vect, 0);
	mlib_status_t push_alias_st =
		mlib_vector_push_back(vect, elem_in_buffer);
	bool push_alias_ok = (push_alias_st == MLIB_SUCCESS) &&
			     (*(int *)mlib_vector_get(vect, 0) == 1337) &&
			     (*(int *)mlib_vector_get(vect, 1) == 1337);
	print_test_result(push_alias_ok, passed_tests, total_tests);

	mlib_vector_reserve(vect, 2);
	printf("%*s", STR_PAD, "Testing insert_at aliased elem forcing growth");
	const int    *ref_first = (const int *)mlib_vector_get(vect, 0);
	mlib_status_t insert_alias_st =
		mlib_vector_insert_at(vect, 1, ref_first);
	bool insert_alias_ok = (insert_alias_st == MLIB_SUCCESS) &&
			       (*(int *)mlib_vector_get(vect, 1) == 1337) &&
			       (mlib_vector_size(vect) == 3);
	print_test_result(insert_alias_ok, passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_insert_at(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Insert At:");

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	printf("%*s", STR_PAD, "Testing insert_at out of bounds");
	int dummy = 42;
	print_test_result(mlib_vector_insert_at(vect, 5, &dummy) ==
				  MLIB_ERR_OUT_OF_BOUNDS,
			  passed_tests, total_tests);

	int val_head = 100;
	printf("%*s", STR_PAD, "Testing insert_at index 0 on empty vector");
	print_test_result(mlib_vector_insert_at(vect, 0, &val_head) ==
				  MLIB_SUCCESS,
			  passed_tests, total_tests);

	int val_tail = 300;
	printf("%*s", STR_PAD, "Testing insert_at at size (delegates push)");
	print_test_result(mlib_vector_insert_at(vect, 1, &val_tail) ==
				  MLIB_SUCCESS,
			  passed_tests, total_tests);

	int val_mid = 200;
	printf("%*s", STR_PAD, "Testing insert_at middle index");
	print_test_result(mlib_vector_insert_at(vect, 1, &val_mid) ==
				  MLIB_SUCCESS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing order after insert_at operations");
	int  expected[] = { 100, 200, 300 };
	bool order_ok = true;
	for (size_t i = 0; i < 3; ++i) {
		int *val = (int *)mlib_vector_get(vect, i);
		if (!val || *val != expected[i]) {
			order_ok = false;
			break;
		}
	}
	print_test_result(order_ok, passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_pop_back(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Pop Back:");

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	printf("%*s", STR_PAD, "Testing pop_back on empty vector");
	print_test_result(mlib_vector_pop_back(vect) == MLIB_ERR_EMPTY,
			  passed_tests, total_tests);

	int v1 = 10, v2 = 20;
	mlib_vector_push_back(vect, &v1);
	mlib_vector_push_back(vect, &v2);

	printf("%*s", STR_PAD, "Testing pop_back on multi-element vector");
	print_test_result(mlib_vector_pop_back(vect) == MLIB_SUCCESS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing back element after pop_back");
	int *back_val = (int *)mlib_vector_back(vect);
	bool back_ok = (back_val != NULL) && (*back_val == 10);
	print_test_result(back_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing pop_back down to empty vector");
	print_test_result(mlib_vector_pop_back(vect) == MLIB_SUCCESS &&
				  mlib_vector_size(vect) == 0,
			  passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_remove_at(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Remove At:");

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	int vals[5] = { 0, 10, 20, 30, 40 };
	for (int i = 0; i < 5; ++i)
		mlib_vector_push_back(vect, &vals[i]);

	printf("%*s", STR_PAD, "Testing remove_at out of bounds");
	print_test_result(mlib_vector_remove_at(vect, 10) ==
				  MLIB_ERR_OUT_OF_BOUNDS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing remove_at middle index (index 2)");
	print_test_result(mlib_vector_remove_at(vect, 2) == MLIB_SUCCESS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing elements order after remove_at");
	int  expected[] = { 0, 10, 30, 40 };
	bool order_ok = true;
	for (size_t i = 0; i < 4; ++i) {
		int *val = (int *)mlib_vector_get(vect, i);
		if (!val || *val != expected[i]) {
			order_ok = false;
			break;
		}
	}
	print_test_result(order_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing remove_at last index (delegates pop)");
	print_test_result(mlib_vector_remove_at(vect, 3) == MLIB_SUCCESS &&
				  mlib_vector_size(vect) == 3 &&
				  *(int *)mlib_vector_back(vect) == 30,
			  passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_get_and_set(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Get and Set:");

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	int vals[3] = { 100, 200, 300 };
	for (int i = 0; i < 3; ++i)
		mlib_vector_push_back(vect, &vals[i]);

	printf("%*s", STR_PAD, "Testing get on out-of-bounds index");
	print_test_result(mlib_vector_get(vect, 3) == NULL, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing set on valid index");
	int replacement = 999;
	print_test_result(mlib_vector_set(vect, 1, &replacement) ==
					  MLIB_SUCCESS &&
				  *(int *)mlib_vector_get(vect, 1) == 999,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing set with NULL (zero-fill slot)");
	print_test_result(mlib_vector_set(vect, 1, NULL) == MLIB_SUCCESS &&
				  *(int *)mlib_vector_get(vect, 1) == 0,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing self-assignment on set");
	void *slot0 = mlib_vector_get(vect, 0);
	print_test_result(mlib_vector_set(vect, 0, slot0) == MLIB_SUCCESS &&
				  *(int *)mlib_vector_get(vect, 0) == 100,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing set on out-of-bounds index");
	print_test_result(mlib_vector_set(vect, 10, &replacement) ==
				  MLIB_ERR_OUT_OF_BOUNDS,
			  passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_capacity_and_resizing(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Capacity and Resizing:");

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	printf("%*s", STR_PAD, "Testing reserve expands capacity");
	print_test_result(mlib_vector_reserve(vect, 64) == MLIB_SUCCESS &&
				  mlib_vector_capacity(vect) == 64 &&
				  mlib_vector_size(vect) == 0,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing resize up (zero-initialized)");
	bool resize_up_ok = (mlib_vector_resize(vect, 8) == MLIB_SUCCESS) &&
			    (mlib_vector_size(vect) == 8);
	for (size_t i = 0; i < 8; ++i) {
		int *val = (int *)mlib_vector_get(vect, i);
		if (!val || *val != 0) {
			resize_up_ok = false;
			break;
		}
	}
	print_test_result(resize_up_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD,
	       "Testing shrink_to_fit adjusts capacity to size");
	print_test_result(mlib_vector_shrink_to_fit(vect) == MLIB_SUCCESS &&
				  mlib_vector_capacity(vect) == 8,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing resize down truncates size");
	print_test_result(mlib_vector_resize(vect, 3) == MLIB_SUCCESS &&
				  mlib_vector_size(vect) == 3,
			  passed_tests, total_tests);

	mlib_vector_clear(vect);
	printf("%*s", STR_PAD, "Testing shrink_to_fit on empty frees buffer");
	print_test_result(mlib_vector_shrink_to_fit(vect) == MLIB_SUCCESS &&
				  mlib_vector_capacity(vect) == 0 &&
				  mlib_vector_data(vect) == NULL,
			  passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_find(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Find:");

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	int vals[5] = { 11, 22, 33, 44, 55 };
	for (int i = 0; i < 5; ++i)
		mlib_vector_push_back(vect, &vals[i]);

	size_t idx = 0;
	int    target = 33;
	printf("%*s", STR_PAD, "Testing find existing element");
	bool find_ok = (mlib_vector_find(vect, &target, cmp_int, &idx) ==
			MLIB_SUCCESS) &&
		       (idx == 2);
	print_test_result(find_ok, passed_tests, total_tests);

	int missing = 999;
	printf("%*s", STR_PAD, "Testing find missing element");
	print_test_result(mlib_vector_find(vect, &missing, cmp_int, &idx) ==
				  MLIB_ERR_NOT_FOUND,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing find on NULL vector or callback");
	print_test_result(mlib_vector_find(NULL, &target, cmp_int, &idx) ==
					  MLIB_ERR_NULL_PTR &&
				  mlib_vector_find(vect, &target, NULL, &idx) ==
					  MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_foreach(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Foreach:");

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	int vals[5] = { 0, 1, 2, 3, 4 };
	for (int i = 0; i < 5; ++i)
		mlib_vector_push_back(vect, &vals[i]);

	int step = 5;
	printf("%*s", STR_PAD, "Testing foreach modification callback");
	print_test_result(mlib_vector_foreach(vect, foreach_increment_cb,
					      &step) == MLIB_SUCCESS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing values updated by foreach");
	bool updated_ok = true;
	for (size_t i = 0; i < 5; ++i) {
		int *val = (int *)mlib_vector_get(vect, i);
		if (!val || *val != (int)i + step) {
			updated_ok = false;
			break;
		}
	}
	print_test_result(updated_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing foreach NULL arguments");
	print_test_result(mlib_vector_foreach(NULL, foreach_increment_cb,
					      NULL) == MLIB_ERR_NULL_PTR &&
				  mlib_vector_foreach(vect, NULL, NULL) ==
					  MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_destructor_semantics(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Destructor Semantics:");

	mlib_vector_t *v_ptrs = create_test_vector(passed_tests, total_tests,
						   sizeof(char *), 0,
						   mlib_free_indirect);
	if (!v_ptrs)
		return;

	char *s1 = test_strdup("Node1");
	char *s2 = test_strdup("Node2");
	char *s3 = test_strdup("Node3");
	mlib_vector_push_back(v_ptrs, &s1);
	mlib_vector_push_back(v_ptrs, &s2);
	mlib_vector_push_back(v_ptrs, &s3);

	printf("%*s", STR_PAD, "Testing pop_back with mlib_free_indirect");
	print_test_result(mlib_vector_pop_back(v_ptrs) == MLIB_SUCCESS &&
				  mlib_vector_size(v_ptrs) == 2,
			  passed_tests, total_tests);

	mlib_vector_destroy(&v_ptrs);

	mlib_vector_t *v_structs = create_test_vector(passed_tests, total_tests,
						      sizeof(test_item_t), 0,
						      test_item_cleanup);
	if (!v_structs)
		return;

	test_item_t item1 = { .id = 1, .tag = test_strdup("Alpha") };
	test_item_t item2 = { .id = 2, .tag = test_strdup("Beta") };
	test_item_t item3 = { .id = 3, .tag = test_strdup("Gamma") };
	mlib_vector_push_back(v_structs, &item1);
	mlib_vector_push_back(v_structs, &item2);
	mlib_vector_push_back(v_structs, &item3);

	printf("%*s", STR_PAD, "Testing resize down with struct cleanup");
	print_test_result(mlib_vector_resize(v_structs, 1) == MLIB_SUCCESS &&
				  mlib_vector_size(v_structs) == 1,
			  passed_tests, total_tests);

	mlib_vector_destroy(&v_structs);
}

void test_clear_and_destroy(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Clear and Destroy:");

	vector_free_call_count = 0;
	mlib_vector_t *vect = create_test_vector(
		passed_tests, total_tests, sizeof(int), 0, create_test_free_fn);
	if (!vect)
		return;

	int dummies[5] = { 1, 2, 3, 4, 5 };
	for (int i = 0; i < 5; ++i)
		mlib_vector_push_back(vect, &dummies[i]);

	printf("%*s", STR_PAD, "Testing clear resets size to 0");
	mlib_vector_clear(vect);
	print_test_result(mlib_vector_size(vect) == 0, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing free_fn invocation count on clear");
	print_test_result(vector_free_call_count == 5, passed_tests,
			  total_tests);

	for (int i = 0; i < 3; ++i)
		mlib_vector_push_back(vect, &dummies[i]);

	printf("%*s", STR_PAD, "Testing destroy cleans elements and sets NULL");
	mlib_vector_destroy(&vect);
	bool destroy_ok = (vect == NULL) && (vector_free_call_count == 8);
	print_test_result(destroy_ok, passed_tests, total_tests);
}

void test_sort(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Sort:");

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	printf("%*s", STR_PAD, "Testing sort on NULL vector or callback");
	print_test_result(
		mlib_vector_sort(NULL, cmp_int) == MLIB_ERR_NULL_PTR &&
			mlib_vector_sort(vect, NULL) == MLIB_ERR_NULL_PTR,
		passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing sort on empty vector");
	print_test_result(mlib_vector_sort(vect, cmp_int) == MLIB_SUCCESS,
			  passed_tests, total_tests);

	int single = 42;
	mlib_vector_push_back(vect, &single);
	printf("%*s", STR_PAD, "Testing sort on 1-element vector");
	print_test_result(mlib_vector_sort(vect, cmp_int) == MLIB_SUCCESS,
			  passed_tests, total_tests);

	int    raw_vals[] = { 45, -3, 100, 0, 12, -3, 8 };
	size_t count = sizeof(raw_vals) / sizeof(raw_vals[0]);
	mlib_vector_clear(vect);
	for (size_t i = 0; i < count; ++i)
		mlib_vector_push_back(vect, &raw_vals[i]);

	printf("%*s", STR_PAD, "Testing full vector sort");
	bool sort_ok = (mlib_vector_sort(vect, cmp_int) == MLIB_SUCCESS);
	int  expected[] = { -3, -3, 0, 8, 12, 45, 100 };
	for (size_t i = 0; i < count; ++i) {
		int *val = (int *)mlib_vector_get(vect, i);
		if (!val || *val != expected[i]) {
			sort_ok = false;
			break;
		}
	}
	print_test_result(sort_ok, passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_sort_range(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Sort Range:");

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	int    raw_vals[] = { 50, 40, 30, 20, 10, 0 };
	size_t count = sizeof(raw_vals) / sizeof(raw_vals[0]);
	for (size_t i = 0; i < count; ++i)
		mlib_vector_push_back(vect, &raw_vals[i]);

	printf("%*s", STR_PAD, "Testing sort_range out of bounds");
	print_test_result(mlib_vector_sort_range(vect, 4, 3, cmp_int) ==
				  MLIB_ERR_OUT_OF_BOUNDS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing sort_range with count < 2 (no-op)");
	print_test_result(mlib_vector_sort_range(vect, 2, 1, cmp_int) ==
				  MLIB_SUCCESS,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing sort_range on sub-interval [1, 5)");
	bool range_ok =
		(mlib_vector_sort_range(vect, 1, 4, cmp_int) == MLIB_SUCCESS);
	int expected[] = { 50, 10, 20, 30, 40, 0 };
	for (size_t i = 0; i < count; ++i) {
		int *val = (int *)mlib_vector_get(vect, i);
		if (!val || *val != expected[i]) {
			range_ok = false;
			break;
		}
	}
	print_test_result(range_ok, passed_tests, total_tests);

	mlib_vector_destroy(&vect);
}

void test_reverse(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Vector Reverse:");

	printf("%*s", STR_PAD, "Testing reverse on NULL vector");
	print_test_result(mlib_vector_reverse(NULL) == MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
						 sizeof(int), 0, NULL);
	if (!vect)
		return;

	printf("%*s", STR_PAD, "Testing reverse on empty vector");
	print_test_result(mlib_vector_reverse(vect) == MLIB_SUCCESS,
			  passed_tests, total_tests);

	int single = 11;
	mlib_vector_push_back(vect, &single);
	printf("%*s", STR_PAD, "Testing reverse on single-element vector");
	print_test_result(mlib_vector_reverse(vect) == MLIB_SUCCESS &&
				  *(int *)mlib_vector_get(vect, 0) == 11,
			  passed_tests, total_tests);

	int vals_odd[] = { 1, 2, 3, 4, 5 };
	mlib_vector_clear(vect);
	for (size_t i = 0; i < 5; ++i)
		mlib_vector_push_back(vect, &vals_odd[i]);

	printf("%*s", STR_PAD, "Testing reverse on odd length (stack buffer)");
	bool odd_ok = (mlib_vector_reverse(vect) == MLIB_SUCCESS);
	for (size_t i = 0; i < 5; ++i) {
		int *val = (int *)mlib_vector_get(vect, i);
		if (!val || *val != (int)(5 - i)) {
			odd_ok = false;
			break;
		}
	}
	print_test_result(odd_ok, passed_tests, total_tests);

	int vals_even[] = { 10, 20, 30, 40 };
	mlib_vector_clear(vect);
	for (size_t i = 0; i < 4; ++i)
		mlib_vector_push_back(vect, &vals_even[i]);

	printf("%*s", STR_PAD, "Testing reverse on even length");
	bool even_ok = (mlib_vector_reverse(vect) == MLIB_SUCCESS);
	int  expected_even[] = { 40, 30, 20, 10 };
	for (size_t i = 0; i < 4; ++i) {
		int *val = (int *)mlib_vector_get(vect, i);
		if (!val || *val != expected_even[i]) {
			even_ok = false;
			break;
		}
	}
	print_test_result(even_ok, passed_tests, total_tests);
	mlib_vector_destroy(&vect);

	/* Test large element swapping (elem_size > 256 bytes forces heap malloc path) */
	mlib_vector_t *v_large = create_test_vector(
		passed_tests, total_tests, sizeof(large_struct_t), 0, NULL);
	if (!v_large)
		return;

	large_struct_t p1, p2;
	memset(p1.data, 0xAA, sizeof(p1.data));
	memset(p2.data, 0xBB, sizeof(p2.data));
	mlib_vector_push_back(v_large, &p1);
	mlib_vector_push_back(v_large, &p2);

	printf("%*s", STR_PAD, "Testing reverse on large elements (heap path)");
	bool large_ok = (mlib_vector_reverse(v_large) == MLIB_SUCCESS);
	large_struct_t *res0 = (large_struct_t *)mlib_vector_get(v_large, 0);
	large_struct_t *res1 = (large_struct_t *)mlib_vector_get(v_large, 1);
	if (!res0 || res0->data[0] != 0xBB || !res1 || res1->data[0] != 0xAA)
		large_ok = false;

	print_test_result(large_ok, passed_tests, total_tests);
	mlib_vector_destroy(&v_large);
}

void test_swap_remove(int *passed_tests, int *total_tests)
{
        puts("Testing MLIB Vector Swap Remove:");

        printf("%*s", STR_PAD, "Testing swap_remove on NULL vector");
        print_test_result(mlib_vector_swap_remove(NULL, 0) ==
                                  MLIB_ERR_NULL_PTR,
                          passed_tests, total_tests);

        mlib_vector_t *vect = create_test_vector(passed_tests, total_tests,
                                                 sizeof(int), 0, NULL);
        if (!vect)
                return;

        printf("%*s", STR_PAD, "Testing swap_remove on empty vector");
        print_test_result(mlib_vector_swap_remove(vect, 0) == MLIB_ERR_EMPTY,
                          passed_tests, total_tests);

        int vals[5] = { 10, 20, 30, 40, 50 };
        for (int i = 0; i < 5; ++i)
                mlib_vector_push_back(vect, &vals[i]);

        printf("%*s", STR_PAD, "Testing swap_remove out of bounds");
        print_test_result(mlib_vector_swap_remove(vect, 5) ==
                                  MLIB_ERR_OUT_OF_BOUNDS,
                          passed_tests, total_tests);

        /* 
         * Test swap_remove pe ultimul element (index 4 -> valoarea 50).
         * Trebuie să delege direct la pop_back fără mutare de elemente.
         */
        printf("%*s", STR_PAD, "Testing swap_remove last index (delegates pop)");
        bool pop_ok = (mlib_vector_swap_remove(vect, 4) == MLIB_SUCCESS) &&
                      (mlib_vector_size(vect) == 4) &&
                      (*(int *)mlib_vector_back(vect) == 40);
        print_test_result(pop_ok, passed_tests, total_tests);

        /* 
         * Vector curent: [10, 20, 30, 40]
         * Eliminăm indexul 1 (valoarea 20).
         * Ultimul element (40) trebuie mutat în locul lui 20.
         * Vector rezultat: [10, 40, 30]
         */
        printf("%*s", STR_PAD, "Testing swap_remove middle index (replaces with last)");
        bool swap_ok = (mlib_vector_swap_remove(vect, 1) == MLIB_SUCCESS) &&
                       (mlib_vector_size(vect) == 3);
        int expected[] = { 10, 40, 30 };
        for (size_t i = 0; i < 3; ++i) {
                int *val = (int *)mlib_vector_get(vect, i);
                if (!val || *val != expected[i]) {
                        swap_ok = false;
                        break;
                }
        }
        print_test_result(swap_ok, passed_tests, total_tests);

        mlib_vector_destroy(&vect);

        /* 
         * Verificare apelare corectă destructor free_fn pe elementul eliminat
         */
        vector_free_call_count = 0;
        mlib_vector_t *vect_dtor = create_test_vector(
                passed_tests, total_tests, sizeof(int), 0, create_test_free_fn);
        if (!vect_dtor)
                return;

        int d1 = 1, d2 = 2;
        mlib_vector_push_back(vect_dtor, &d1);
        mlib_vector_push_back(vect_dtor, &d2);

        mlib_vector_swap_remove(vect_dtor, 0);

        printf("%*s", STR_PAD, "Testing swap_remove triggers free_fn");
        bool dtor_ok = (vector_free_call_count == 1) &&
                       (mlib_vector_size(vect_dtor) == 1) &&
                       (*(int *)mlib_vector_get(vect_dtor, 0) == 2);
        print_test_result(dtor_ok, passed_tests, total_tests);

        mlib_vector_destroy(&vect_dtor);
}

int main(void)
{
	int passed_tests = 0, total_tests = 0;

	test_create(&passed_tests, &total_tests);
	test_push_back(&passed_tests, &total_tests);
	test_self_insertion_aliasing(&passed_tests, &total_tests);
	test_insert_at(&passed_tests, &total_tests);
	test_pop_back(&passed_tests, &total_tests);
	test_remove_at(&passed_tests, &total_tests);
	test_get_and_set(&passed_tests, &total_tests);
	test_capacity_and_resizing(&passed_tests, &total_tests);
	test_find(&passed_tests, &total_tests);
	test_foreach(&passed_tests, &total_tests);
	test_destructor_semantics(&passed_tests, &total_tests);
	test_clear_and_destroy(&passed_tests, &total_tests);
	test_sort(&passed_tests, &total_tests);
	test_sort_range(&passed_tests, &total_tests);
	test_reverse(&passed_tests, &total_tests);
	test_swap_remove(&passed_tests, &total_tests);

	printf("\n==================================================\n");
	printf("SUMMARY: %d / %d tests passed\n", passed_tests, total_tests);
	printf("==================================================\n");

	return (passed_tests == total_tests) ? 0 : 1;
}
