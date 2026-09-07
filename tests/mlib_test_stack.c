#include <mlib/common.h>
#include <mlib/mlib_stack.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_FAILED "\033[31mFAILED\033[0m"
#define TEST_PASSED "\033[32mPASSED\033[0m"
#define STR_PAD	    -60

#define TEST_ITEMS_COUNT 128

static int stack_free_call_count = 0;

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
	MLIB_UNUSED(elem);
	++stack_free_call_count;
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

static mlib_stack_t *create_test_stack(int *passed_tests, int *total_tests,
				       size_t elem_size,
				       size_t initial_capacity, mlib_free_fn fn)
{
	mlib_stack_t *stack =
		mlib_stack_create(elem_size, initial_capacity, fn);
	if (!stack) {
		print_test_result(false, passed_tests, total_tests);
		puts("Not enough memory available to run the test");
		return NULL;
	}
	return stack;
}

void test_create(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Stack Create:");

	printf("%*s", STR_PAD, "Testing elem_size == 0 fails");
	print_test_result(mlib_stack_create(0, 10, NULL) == NULL, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing capacity overflow on create fails");
	print_test_result(mlib_stack_create(sizeof(int), SIZE_MAX, NULL) ==
				  NULL,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Creating stack with initial capacity 0");
	mlib_stack_t *stack = create_test_stack(
		passed_tests, total_tests, sizeof(int), 0, create_test_free_fn);
	if (!stack)
		return;

	print_test_result(true, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing initial size == 0");
	print_test_result(mlib_stack_size(stack) == 0, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing initial capacity == 0");
	print_test_result(mlib_stack_capacity(stack) == 0, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing stack is empty on creation");
	print_test_result(mlib_stack_is_empty(stack) == true, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing peek on empty stack returns NULL");
	print_test_result(mlib_stack_peek(stack) == NULL, passed_tests,
			  total_tests);

	mlib_stack_destroy(&stack);
}

void test_push_and_peek(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Stack Push and Peek:");

	printf("%*s", STR_PAD, "Testing push on NULL stack");
	int dummy = 42;
	print_test_result(mlib_stack_push(NULL, &dummy) == MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	mlib_stack_t *stack = create_test_stack(passed_tests, total_tests,
						sizeof(int), 0, NULL);
	if (!stack)
		return;

	int  vals[TEST_ITEMS_COUNT];
	bool push_ok = true;
	for (int i = 0; i < TEST_ITEMS_COUNT; ++i) {
		vals[i] = i * 10;
		if (mlib_stack_push(stack, &vals[i]) != MLIB_SUCCESS) {
			push_ok = false;
			break;
		}
	}

	printf("%*s", STR_PAD, "Testing push return status");
	print_test_result(push_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing size after multiple pushes");
	print_test_result(mlib_stack_size(stack) == TEST_ITEMS_COUNT,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing is_empty after pushes");
	print_test_result(mlib_stack_is_empty(stack) == false, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing peek returns last pushed element");
	int *top_val = (int *)mlib_stack_peek(stack);
	bool peek_ok = (top_val != NULL) &&
		       (*top_val == (TEST_ITEMS_COUNT - 1) * 10);
	print_test_result(peek_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing push NULL (zero-fill slot)");
	bool null_push_ok = (mlib_stack_push(stack, NULL) == MLIB_SUCCESS) &&
			    (mlib_stack_size(stack) == TEST_ITEMS_COUNT + 1);
	int *zero_top = (int *)mlib_stack_peek(stack);
	null_push_ok = null_push_ok && (zero_top != NULL) && (*zero_top == 0);
	print_test_result(null_push_ok, passed_tests, total_tests);

	mlib_stack_destroy(&stack);
}

void test_pop_and_lifo_order(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Stack Pop and LIFO Order:");

	printf("%*s", STR_PAD, "Testing pop on NULL stack");
	print_test_result(mlib_stack_pop(NULL) == MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	mlib_stack_t *stack = create_test_stack(passed_tests, total_tests,
						sizeof(int), 0, NULL);
	if (!stack)
		return;

	printf("%*s", STR_PAD, "Testing pop on empty stack");
	print_test_result(mlib_stack_pop(stack) == MLIB_ERR_EMPTY, passed_tests,
			  total_tests);

	int vals[5] = { 10, 20, 30, 40, 50 };
	for (int i = 0; i < 5; ++i)
		mlib_stack_push(stack, &vals[i]);

	printf("%*s", STR_PAD, "Testing strict LIFO extraction order");
	bool lifo_ok = true;
	for (int i = 4; i >= 0; --i) {
		int *top = (int *)mlib_stack_peek(stack);
		if (!top || *top != vals[i]) {
			lifo_ok = false;
			break;
		}
		if (mlib_stack_pop(stack) != MLIB_SUCCESS) {
			lifo_ok = false;
			break;
		}
	}
	print_test_result(lifo_ok, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing size after popping all elements");
	print_test_result(mlib_stack_size(stack) == 0 &&
				  mlib_stack_is_empty(stack) == true,
			  passed_tests, total_tests);

	mlib_stack_destroy(&stack);
}

void test_capacity_and_resizing(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Stack Capacity and Resizing:");

	mlib_stack_t *stack = create_test_stack(passed_tests, total_tests,
						sizeof(int), 0, NULL);
	if (!stack)
		return;

	printf("%*s", STR_PAD, "Testing reserve expands capacity");
	print_test_result(mlib_stack_reserve(stack, 32) == MLIB_SUCCESS &&
				  mlib_stack_capacity(stack) >= 32 &&
				  mlib_stack_size(stack) == 0,
			  passed_tests, total_tests);

	int val = 99;
	mlib_stack_push(stack, &val);

	printf("%*s", STR_PAD,
	       "Testing shrink_to_fit reduces capacity to size");
	print_test_result(mlib_stack_shrink_to_fit(stack) == MLIB_SUCCESS &&
				  mlib_stack_capacity(stack) == 1 &&
				  mlib_stack_size(stack) == 1,
			  passed_tests, total_tests);

	mlib_stack_clear(stack);
	printf("%*s", STR_PAD,
	       "Testing shrink_to_fit on empty stack frees buffer");
	print_test_result(mlib_stack_shrink_to_fit(stack) == MLIB_SUCCESS &&
				  mlib_stack_capacity(stack) == 0 &&
				  mlib_stack_size(stack) == 0,
			  passed_tests, total_tests);

	mlib_stack_destroy(&stack);
}

void test_destructor_semantics(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Stack Destructor Semantics:");

	mlib_stack_t *stack_ptrs = create_test_stack(passed_tests, total_tests,
						     sizeof(char *), 0,
						     mlib_free_indirect);
	if (!stack_ptrs)
		return;

	char *s1 = test_strdup("StackNode1");
	char *s2 = test_strdup("StackNode2");
	mlib_stack_push(stack_ptrs, &s1);
	mlib_stack_push(stack_ptrs, &s2);

	printf("%*s", STR_PAD, "Testing pop cleans pointer via destructor");
	print_test_result(mlib_stack_pop(stack_ptrs) == MLIB_SUCCESS &&
				  mlib_stack_size(stack_ptrs) == 1,
			  passed_tests, total_tests);

	mlib_stack_destroy(&stack_ptrs);

	stack_free_call_count = 0;
	mlib_stack_t *stack_cb = create_test_stack(
		passed_tests, total_tests, sizeof(int), 0, create_test_free_fn);
	if (!stack_cb)
		return;

	int dummies[4] = { 1, 2, 3, 4 };
	for (int i = 0; i < 4; ++i)
		mlib_stack_push(stack_cb, &dummies[i]);

	printf("%*s", STR_PAD, "Testing clear invokes destructor on all items");
	mlib_stack_clear(stack_cb);
	print_test_result(stack_free_call_count == 4 &&
				  mlib_stack_size(stack_cb) == 0,
			  passed_tests, total_tests);

	for (int i = 0; i < 3; ++i)
		mlib_stack_push(stack_cb, &dummies[i]);

	printf("%*s", STR_PAD, "Testing destroy frees elements and zeroes ptr");
	mlib_stack_destroy(&stack_cb);
	bool destroy_ok = (stack_cb == NULL) && (stack_free_call_count == 7);
	print_test_result(destroy_ok, passed_tests, total_tests);
}

void test_null_guards(int *passed_tests, int *total_tests)
{
	puts("Testing MLIB Stack NULL Pointer Guards:");

	printf("%*s", STR_PAD, "Testing size on NULL stack returns 0");
	print_test_result(mlib_stack_size(NULL) == 0, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing capacity on NULL stack returns 0");
	print_test_result(mlib_stack_capacity(NULL) == 0, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing is_empty on NULL stack returns true");
	print_test_result(mlib_stack_is_empty(NULL) == true, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing peek on NULL stack returns NULL");
	print_test_result(mlib_stack_peek(NULL) == NULL, passed_tests,
			  total_tests);

	printf("%*s", STR_PAD, "Testing reserve on NULL stack returns ERR");
	print_test_result(mlib_stack_reserve(NULL, 10) == MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing shrink_to_fit on NULL returns ERR");
	print_test_result(mlib_stack_shrink_to_fit(NULL) == MLIB_ERR_NULL_PTR,
			  passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing clear on NULL stack is safe");
	mlib_stack_clear(NULL);
	print_test_result(true, passed_tests, total_tests);

	printf("%*s", STR_PAD, "Testing destroy on NULL reference is safe");
	mlib_stack_destroy(NULL);
	mlib_stack_t *null_stack = NULL;
	mlib_stack_destroy(&null_stack);
	print_test_result(true, passed_tests, total_tests);
}

int main(void)
{
	int passed_tests = 0, total_tests = 0;

	test_create(&passed_tests, &total_tests);
	test_push_and_peek(&passed_tests, &total_tests);
	test_pop_and_lifo_order(&passed_tests, &total_tests);
	test_capacity_and_resizing(&passed_tests, &total_tests);
	test_destructor_semantics(&passed_tests, &total_tests);
	test_null_guards(&passed_tests, &total_tests);

	printf("\n==================================================\n");
	printf("SUMMARY: %d / %d tests passed\n", passed_tests, total_tests);
	printf("==================================================\n");

	return (passed_tests == total_tests) ? 0 : 1;
}
