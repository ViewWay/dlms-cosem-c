#ifndef TEST_MACROS_H
#define TEST_MACROS_H

#include <stdio.h>
#include <string.h>

extern int current_suite_failed;

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("  FAIL: %s != %s (%d vs %d) at %s:%d\n", #a, #b, (int)(a), (int)(b), __FILE__, __LINE__); \
        current_suite_failed = 1; return; \
    } \
} while(0)

#define ASSERT_NEQ(a, b) do { \
    if ((a) == (b)) { \
        printf("  FAIL: %s == %s at %s:%d\n", #a, #b, __FILE__, __LINE__); \
        current_suite_failed = 1; return; \
    } \
} while(0)

#define ASSERT_OK(ret) ASSERT_EQ((ret), 0)

#define ASSERT_MEM_EQ(a, b, len) do { \
    if (memcmp((a), (b), (len)) != 0) { \
        printf("  FAIL: memory mismatch at %s:%d\n", __FILE__, __LINE__); \
        current_suite_failed = 1; return; \
    } \
} while(0)

#define ASSERT_TRUE(x) ASSERT_EQ((x), 1)
#define ASSERT_FALSE(x) ASSERT_EQ((x), 0)

#define ASSERT_LT(a, b) do { \
    if (!((a) < (b))) { \
        printf("  FAIL: %s >= %s at %s:%d\n", #a, #b, __FILE__, __LINE__); \
        current_suite_failed = 1; return; \
    } \
} while(0)

#define ASSERT_GT(a, b) ASSERT_LT((b), (a))

#endif
