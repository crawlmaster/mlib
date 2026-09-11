#include <mlib/common.h>
#include <mlib/mlib_queue.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_FAILED "\033[31mFAILED\033[0m"
#define TEST_PASSED "\033[32mPASSED\033[0m"
#define STR_PAD	    -50

#define TEST_ITEMS_COUNT 16

static int queue_free_call_count = 0;

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

static void count_free_fn(void *elem)
{
	MLIB_UNUSED(elem);
	++queue_free_call_count;
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

static mlib_queue_t *create_test_queue(int *passed_tests, int *total_tests,
				       size_t elem_size,
				       size_t initial_capacity, mlib_free_fn fn)
{
	mlib_queue_t *queue =
		mlib_queue_create(elem_size, initial_capacity, fn);
	if (!queue) {
		print_test_result(false, passed_tests, total_tests);
		puts("Not enough memory available to run test");
		return NULL;
	}
	return queue;
}

static void test_create_and_pow2_alignment(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Queue Create & Power of Two Alignment:");

	printf("%*s", STR_PAD, "Testing elem_size == 0 fails");
	print_test_result(mlib_queue_create(0, 10, NULL) == NULL, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing capacity overflow on create fails");
	print_test_result(mlib_queue_create(sizeof(int), SIZE_MAX, NULL) ==
				  NULL,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Creating queue with capacity 0 (lazy)");
	mlib_queue_t *q0 = create_test_queue(passed_tests, total_tests,
					     sizeof(int), 0, NULL);
	if (!q0)
		return;
	print_test_result(true, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing initial size == 0");
	print_test_result(mlib_queue_size(q0) == 0 && mlib_queue_is_empty(q0),
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing initial capacity == 0");
	print_test_result(mlib_queue_capacity(q0) == 0, passed_tests,
			  total_tests);
	mlib_queue_destroy(&q0);

	printf("%*s", STR_PAD, "Testing arbitrary capacity rounds up to pow2");
	mlib_queue_t *q_arb = create_test_queue(passed_tests, total_tests,
						sizeof(int), 10, NULL);
	if (!q_arb)
		return;
	print_test_result(mlib_queue_capacity(q_arb) == 16, passed_tests,
			  total_tests);
	mlib_queue_destroy(&q_arb);
}

static void test_push_peek_and_fifo_order(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Queue Push, Peek & FIFO Order:");

	mlib_queue_t *queue = create_test_queue(passed_tests, total_tests,
						sizeof(int), 8, NULL);
	if (!queue)
		return;

	int  vals[TEST_ITEMS_COUNT];
	bool push_ok = true;
	for (int i = 0; i < TEST_ITEMS_COUNT; ++i) {
		vals[i] = i * 100;
		if (mlib_queue_push(queue, &vals[i]) != MLIB_SUCCESS) {
			push_ok = false;
			break;
		}
	}

	printf("%*s", STR_PAD,
	       "Testing bulk push with dynamic capacity doubling");
	print_test_result(push_ok && mlib_queue_size(queue) == TEST_ITEMS_COUNT,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing peek returns front element");
	int *front = (int *)mlib_queue_peek(queue);
	print_test_result(front != NULL && *front == vals[0], passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing strict FIFO order during dequeue");
	bool fifo_ok = true;
	for (int i = 0; i < TEST_ITEMS_COUNT; ++i) {
		int *cur = (int *)mlib_queue_peek(queue);
		if (!cur || *cur != vals[i]) {
			fifo_ok = false;
			break;
		}
		if (mlib_queue_pop(queue) != MLIB_SUCCESS) {
			fifo_ok = false;
			break;
		}
	}
	print_test_result(fifo_ok && mlib_queue_is_empty(queue), passed_tests,
			  total_tests);

	mlib_queue_destroy(&queue);
}

static void test_wraparound_and_realloc_unwrapping(int *passed_tests,
						   int *total_tests)
{
	puts("Testing Ring Buffer Wraparound & Reorder Logic:");

	mlib_queue_t *queue = create_test_queue(passed_tests, total_tests,
						sizeof(int), 8, NULL);
	if (!queue)
		return;

	for (int i = 0; i < 6; ++i)
		mlib_queue_push(queue, &i);

	for (int i = 0; i < 4; ++i)
		mlib_queue_pop(queue);

	for (int i = 100; i < 106; ++i)
		mlib_queue_push(queue, &i);

	printf("%*s", STR_PAD,
	       "Testing buffer reaches full state with wrapped head");
	print_test_result(mlib_queue_size(queue) == 8 &&
				  mlib_queue_capacity(queue) == 8,
			  passed_tests, total_tests);

	int overflow_val = 999;
	printf("%*s", STR_PAD, "Pushing when wrapped triggers un-wrap realloc");
	bool push_wrap_ok =
		(mlib_queue_push(queue, &overflow_val) == MLIB_SUCCESS);
	print_test_result(push_wrap_ok && mlib_queue_capacity(queue) == 16,
			  passed_tests, total_tests);

	int  expected_vals[] = { 4, 5, 100, 101, 102, 103, 104, 105, 999 };
	bool order_ok = true;
	for (size_t i = 0; i < sizeof(expected_vals) / sizeof(expected_vals[0]);
	     ++i) {
		int *cur = (int *)mlib_queue_peek(queue);
		if (!cur || *cur != expected_vals[i]) {
			order_ok = false;
			break;
		}
		mlib_queue_pop(queue);
	}

	printf("%*s", STR_PAD, "Testing full data integrity post un-wrapping");
	print_test_result(order_ok && mlib_queue_is_empty(queue), passed_tests,
			  total_tests);

	mlib_queue_destroy(&queue);
}

static void test_aliasing_push_under_realloc(int *passed_tests,
					     int *total_tests)
{
	puts("Testing Self-Aliasing Push during Buffer Resize:");

	/* Case 1: Aliasing from Segment A (front element: 4) */
	mlib_queue_t *queue = create_test_queue(passed_tests, total_tests,
						sizeof(int), 8, NULL);
	if (!queue)
		return;

	for (int i = 0; i < 6; ++i)
		mlib_queue_push(queue, &i);
	for (int i = 0; i < 4; ++i)
		mlib_queue_pop(queue);
	for (int i = 100; i < 106; ++i)
		mlib_queue_push(queue, &i);

	int *segA_ptr = (int *)mlib_queue_peek(queue);
	printf("%*s", STR_PAD,
	       "Testing aliased push from Segment A during resize");
	bool alias_segA_ok = (mlib_queue_push(queue, segA_ptr) == MLIB_SUCCESS);
	print_test_result(alias_segA_ok, passed_tests, total_tests);

	for (int i = 0; i < 8; ++i)
		mlib_queue_pop(queue);

	int *last_elem_a = (int *)mlib_queue_peek(queue);
	printf("%*s", STR_PAD,
	       "Verifying Segment A aliased value preserved (4)");
	print_test_result(last_elem_a != NULL && *last_elem_a == 4,
			  passed_tests, total_tests);
	mlib_queue_destroy(&queue);

	/*
	 * Case 2: Aliasing from Segment B (relocated elements at physical index 0..head-1)
	 * We obtain a pointer to index 0 when head is 0, preserve that pointer address,
	 * wrap the buffer around it, and then trigger a resize using that exact pointer.
	 */
	queue = create_test_queue(passed_tests, total_tests, sizeof(int), 8,
				  NULL);
	if (!queue)
		return;

	int anchor = 777;
	mlib_queue_push(queue, &anchor);
	/* Pointer to physical slot 0 */
	int *segB_ptr = (int *)mlib_queue_peek(queue);

	/* Fill remaining 7 elements to reach capacity 8 */
	for (int i = 1; i < 8; ++i)
		mlib_queue_push(queue, &i);

	/*
	 * Pop 4 elements. Head advances to index 4.
	 * Physical slot 0 now holds old/overwritten data.
	 * Push 4 new elements: [200, 201, 202, 203].
	 * Element 200 wraps around and lands precisely at physical index 0!
	 */
	for (int i = 0; i < 4; ++i)
		mlib_queue_pop(queue);

	for (int i = 200; i < 204; ++i)
		mlib_queue_push(queue, &i);

	/*
	 * Queue is now full (size == 8, capacity == 8, head == 4).
	 * segB_ptr points to physical index 0, which holds 200 (Segment B).
	 * Pushing segB_ptr triggers realloc, un-wrapping Segment B to old_capacity.
	 */
	printf("%*s", STR_PAD,
	       "Testing aliased push from Segment B during resize");
	bool alias_segB_ok = (mlib_queue_push(queue, segB_ptr) == MLIB_SUCCESS);
	print_test_result(alias_segB_ok, passed_tests, total_tests);

	for (int i = 0; i < 8; ++i)
		mlib_queue_pop(queue);

	int *last_elem_b = (int *)mlib_queue_peek(queue);
	printf("%*s", STR_PAD,
	       "Verifying Segment B aliased value preserved (200)");
	print_test_result(last_elem_b != NULL && *last_elem_b == 200,
			  passed_tests, total_tests);

	mlib_queue_destroy(&queue);
}

static void test_hysteresis_shrinking(int *passed_tests, int *total_tests)
{
	puts("Testing Hysteresis Shrinking on Pop:");

	mlib_queue_t *queue = create_test_queue(passed_tests, total_tests,
						sizeof(int), 8, NULL);
	if (!queue)
		return;

	for (int i = 0; i < 32; ++i)
		mlib_queue_push(queue, &i);

	printf("%*s", STR_PAD, "Testing queue expanded to capacity 32");
	print_test_result(mlib_queue_capacity(queue) == 32 &&
				  mlib_queue_size(queue) == 32,
			  passed_tests, total_tests);

	for (int i = 0; i < 24; ++i)
		mlib_queue_pop(queue);

	printf("%*s", STR_PAD,
	       "Testing buffer shrunk to capacity 16 at <= 1/4");
	print_test_result(mlib_queue_capacity(queue) == 16 &&
				  mlib_queue_size(queue) == 8,
			  passed_tests, total_tests);

	while (!mlib_queue_is_empty(queue))
		mlib_queue_pop(queue);

	printf("%*s", STR_PAD, "Testing buffer preserves min capacity (8)");
	print_test_result(mlib_queue_capacity(queue) == 8 &&
				  mlib_queue_size(queue) == 0,
			  passed_tests, total_tests);

	mlib_queue_destroy(&queue);
}

static void test_destructor_semantics(int *passed_tests, int *total_tests)
{
	puts("Testing Destructor Semantics on Queue:");

	mlib_queue_t *queue = create_test_queue(passed_tests, total_tests,
						sizeof(char *), 8,
						mlib_free_indirect);
	if (!queue)
		return;

	char *s1 = test_strdup("QueueElement1");
	char *s2 = test_strdup("QueueElement2");
	mlib_queue_push(queue, &s1);
	mlib_queue_push(queue, &s2);

	printf("%*s", STR_PAD, "Testing pop cleans pointer via destructor");
	print_test_result(mlib_queue_pop(queue) == MLIB_SUCCESS &&
				  mlib_queue_size(queue) == 1,
			  passed_tests, total_tests);

	mlib_queue_destroy(&queue);

	queue_free_call_count = 0;
	mlib_queue_t *q_cb = create_test_queue(passed_tests, total_tests,
					       sizeof(int), 8, count_free_fn);
	if (!q_cb)
		return;

	int dummies[6] = { 1, 2, 3, 4, 5, 6 };
	for (int i = 0; i < 6; ++i)
		mlib_queue_push(q_cb, &dummies[i]);

	printf("%*s", STR_PAD, "Testing clear invokes destructor on all items");
	mlib_queue_clear(q_cb);
	print_test_result(queue_free_call_count == 6 &&
				  mlib_queue_size(q_cb) == 0,
			  passed_tests, total_tests);

	for (int i = 0; i < 3; ++i)
		mlib_queue_push(q_cb, &dummies[i]);

	printf("%*s", STR_PAD, "Testing destroy frees elements and zeroes ptr");
	mlib_queue_destroy(&q_cb);
	print_test_result(q_cb == NULL && queue_free_call_count == 9,
			  passed_tests, total_tests);
}

static void test_null_guards(int *passed_tests, int *total_tests)
{
	puts("Testing NULL Pointer Guards:");

	int dummy = 100;
	printf("%*s", STR_PAD, "Testing push on NULL returns ERR");
	print_test_result(mlib_queue_push(NULL, &dummy) == MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing pop on NULL returns ERR");
	print_test_result(mlib_queue_pop(NULL) == MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing peek on NULL returns NULL");
	print_test_result(mlib_queue_peek(NULL) == NULL, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing size on NULL returns 0");
	print_test_result(mlib_queue_size(NULL) == 0, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing capacity on NULL returns 0");
	print_test_result(mlib_queue_capacity(NULL) == 0, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing is_empty on NULL returns true");
	print_test_result(mlib_queue_is_empty(NULL) == true, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing clear on NULL is safe");
	mlib_queue_clear(NULL);
	print_test_result(true, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing destroy on NULL is safe");
	mlib_queue_destroy(NULL);
	mlib_queue_t *null_q = NULL;
	mlib_queue_destroy(&null_q);
	print_test_result(true, passed_tests, total_tests);
}

int main(void)
{
	int passed_tests = 0, total_tests = 0;

	test_create_and_pow2_alignment(&passed_tests, &total_tests);
	test_push_peek_and_fifo_order(&passed_tests, &total_tests);
	test_wraparound_and_realloc_unwrapping(&passed_tests, &total_tests);
	test_aliasing_push_under_realloc(&passed_tests, &total_tests);
	test_hysteresis_shrinking(&passed_tests, &total_tests);
	test_destructor_semantics(&passed_tests, &total_tests);
	test_null_guards(&passed_tests, &total_tests);

	printf("\n==================================================\n");
	printf("SUMMARY: %d / %d tests passed\n", passed_tests, total_tests);
	printf("==================================================\n");

	return (passed_tests == total_tests) ? 0 : 1;
}
