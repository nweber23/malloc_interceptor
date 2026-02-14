#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>

/* Forward declarations for alignment functions */
int posix_memalign(void **memptr, size_t alignment, size_t size);
void *aligned_alloc(size_t alignment, size_t size);
void *memalign(size_t alignment, size_t size);
void *valloc(size_t size);
void *pvalloc(size_t size);

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(cond, msg) do { \
    tests_run++; \
    if (cond) { \
        tests_passed++; \
    } else { \
        tests_failed++; \
        fprintf(stderr, "FAIL: %s\n", msg); \
    } \
} while(0)

#define ASSERT_NULL(ptr, msg) ASSERT_TRUE((ptr) == NULL, msg)
#define ASSERT_NOT_NULL(ptr, msg) ASSERT_TRUE((ptr) != NULL, msg)

void test_malloc_basic(void) {
    fprintf(stderr, "\n=== Test: malloc basic ===\n");

    /* Test 1: Normal malloc should succeed */
    void *p = malloc(100);
    ASSERT_NOT_NULL(p, "malloc(100) should succeed");
    if (p) free(p);

    /* Test 2: Multiple allocations */
    for (int i = 0; i < 10; i++) {
        void *tmp = malloc(50 + i);
        ASSERT_NOT_NULL(tmp, "malloc in loop should succeed");
        if (tmp) free(tmp);
    }
}

void test_malloc_fail_at(void) {
    fprintf(stderr, "\n=== Test: malloc with MALLOC_FAIL_AT ===\n");
    /* This test is typically run with environment variable set externally */
    fprintf(stderr, "Note: Run with MALLOC_FAIL_AT=1 to test\n");
}

void test_calloc_basic(void) {
    fprintf(stderr, "\n=== Test: calloc basic ===\n");

    void *p = calloc(10, 20);
    ASSERT_NOT_NULL(p, "calloc(10, 20) should succeed");
    if (p) free(p);
}

void test_realloc_basic(void) {
    fprintf(stderr, "\n=== Test: realloc basic ===\n");

    void *p = malloc(50);
    ASSERT_NOT_NULL(p, "initial malloc should succeed");

    if (p) {
        void *q = realloc(p, 100);
        ASSERT_NOT_NULL(q, "realloc should succeed");
        if (q) free(q);
    }
}

void test_posix_memalign(void) {
    fprintf(stderr, "\n=== Test: posix_memalign ===\n");

    void *p = NULL;
    int ret = posix_memalign(&p, 16, 100);
    ASSERT_TRUE(ret == 0, "posix_memalign should succeed");
    if (p) free(p);
}

void test_aligned_alloc(void) {
    fprintf(stderr, "\n=== Test: aligned_alloc ===\n");

    void *p = aligned_alloc(16, 100);
    ASSERT_NOT_NULL(p, "aligned_alloc should succeed");
    if (p) free(p);
}

void test_memalign(void) {
    fprintf(stderr, "\n=== Test: memalign ===\n");

    void *p = memalign(16, 100);
    ASSERT_NOT_NULL(p, "memalign should succeed");
    if (p) free(p);
}

void test_valloc(void) {
    fprintf(stderr, "\n=== Test: valloc ===\n");

    void *p = valloc(100);
    ASSERT_NOT_NULL(p, "valloc should succeed");
    if (p) free(p);
}

void test_pvalloc(void) {
    fprintf(stderr, "\n=== Test: pvalloc ===\n");

    void *p = pvalloc(100);
    ASSERT_NOT_NULL(p, "pvalloc should succeed");
    if (p) free(p);
}

void test_size_filtering(void) {
    fprintf(stderr, "\n=== Test: size-based filtering ===\n");
    /* This test is typically run with MALLOC_FAIL_SIZE_MIN/MAX environment variables */
    fprintf(stderr, "Note: Run with MALLOC_FAIL_SIZE_MIN=1024 or MALLOC_FAIL_SIZE_MAX=100 to test\n");
}

void test_errno_on_fail(void) {
    fprintf(stderr, "\n=== Test: errno on allocation failure ===\n");
    /* This test is typically run with MALLOC_FAIL_AT=1 to trigger a failure */
    fprintf(stderr, "Note: Run with MALLOC_FAIL_AT=1 to verify errno is set to ENOMEM\n");
}

/* Thread-safe test */
static void *thread_malloc_test(void *arg) {
    int thread_id = (intptr_t)arg;
    for (int i = 0; i < 100; i++) {
        void *p = malloc(100 + (thread_id * 10) + i);
        if (p) {
            free(p);
        }
    }
    return NULL;
}

void test_thread_safety(void) {
    fprintf(stderr, "\n=== Test: thread safety ===\n");

    pthread_t threads[4];
    for (int i = 0; i < 4; i++) {
        int ret = pthread_create(&threads[i], NULL, thread_malloc_test, (void *)(intptr_t)i);
        ASSERT_TRUE(ret == 0, "pthread_create should succeed");
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main(void) {
    fprintf(stderr, "====================================\n");
    fprintf(stderr, "Malloc Interceptor Test Suite\n");
    fprintf(stderr, "====================================\n");

    test_malloc_basic();
    test_malloc_fail_at();
    test_calloc_basic();
    test_realloc_basic();
    test_posix_memalign();
    test_aligned_alloc();
    test_memalign();
    test_valloc();
    test_pvalloc();
    test_size_filtering();
    test_errno_on_fail();
    test_thread_safety();

    fprintf(stderr, "\n====================================\n");
    fprintf(stderr, "Test Results: %d/%d passed\n", tests_passed, tests_run);
    fprintf(stderr, "====================================\n");

    return tests_failed > 0 ? 1 : 0;
}
