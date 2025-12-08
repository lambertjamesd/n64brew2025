#ifndef __TEST_FRAMEWORK_TEST_H__
#define __TEST_FRAMEWORK_TEST_H__

#include <setjmp.h>
#include <stdbool.h>
#include <libdragon.h>
#include "../math/vector2.h"

// two macros ensures any macro passed will
// be expanded before being stringified
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

struct test_context {
    jmp_buf jump;
};

typedef void (*test_callback)(struct test_context* context);

void test_run_raw(test_callback callback, const char* name);
#define test_run(fn) test_run_raw(fn, #fn)

void test_fatal_raw(struct test_context* t, const char* message, const char* location);

void test_eqi_raw(struct test_context* t, int expected, int actual, const char* location);
void test_neqi_raw(struct test_context* t, int expected, int actual, const char* location);

void test_ltf_raw(struct test_context* t, float expected, float actual, const char* location);
void test_ltef_raw(struct test_context* t, float expected, float actual, const char* location);
void test_gtf_raw(struct test_context* t, float expected, float actual, const char* location);
void test_gtef_raw(struct test_context* t, float expected, float actual, const char* location);
void test_near_equalf_raw(struct test_context* t, float expected, float actual, const char* location);

void test_vec2_equal_raw(struct test_context* t, struct Vector2* expected, struct Vector2* actual, const char* location);

#define test_fatal(t, message) test_fatal_raw(t, message, __FILE__ ":" STRINGIZE(__LINE__))

#define test_eqi(t, expected, actual) test_eqi_raw(t, expected, actual, __FILE__ ":" STRINGIZE(__LINE__))
#define test_neqi(t, expected, actual) test_neqi_raw(t, expected, actual, __FILE__ ":" STRINGIZE(__LINE__))

#define test_ltf(t, left, right) test_ltf_raw(t, left, right, __FILE__ ":" STRINGIZE(__LINE__))
#define test_ltef(t, left, right) test_ltef_raw(t, left, right, __FILE__ ":" STRINGIZE(__LINE__))
#define test_gtf(t, left, right) test_gtf_raw(t, left, right, __FILE__ ":" STRINGIZE(__LINE__))
#define test_gtef(t, left, right) test_gtef_raw(t, left, right, __FILE__ ":" STRINGIZE(__LINE__))
#define test_near_equalf(t, expected, actual) test_near_equalf_raw(t, expected, actual, __FILE__ ":" STRINGIZE(__LINE__))

#define test_vec2_equal(t, expected, actual) test_vec2_equal_raw(t, expected, actual, __FILE__ ":" STRINGIZE(__LINE__))

void test_report_failures();

void test_load_scene(const char* name);

typedef void (*test_deferred_call)(void* data);

void test_defer_call(struct test_context* t, test_deferred_call callback, void* data);

#endif