#include "framework_test.h"

#include <stdio.h>
#include <string.h>
#include "../scene/scene.h"
#include "../scene/scene_loader.h"

#define MAX_NAMED_FAILS 16

static int total_test_count;
static int failed_test_count;

static const char* failed_test_names[MAX_NAMED_FAILS];

void test_call_deferred();

void test_run_raw(test_callback callback, const char* name) {
    struct test_context context;
    total_test_count += 1;
    if (setjmp(context.jump)) {
        fprintf(stderr, "TEST FAIL %s\n", name);

        if (failed_test_count < MAX_NAMED_FAILS) {
            failed_test_names[failed_test_count] = name;
            failed_test_count += 1;
        }

    } else {
        fprintf(stderr, "TEST START %s...\n", name);
        callback(&context);
        fprintf(stderr, "TEST PASS %s\n", name);
    }
    test_call_deferred();
}

void test_fatal_raw(struct test_context* t, const char* message, const char* location) {
    fprintf(stderr, "FATAL %s at %s\n", message, location);
    longjmp(t->jump, 1);
}

void test_eqi_raw(struct test_context* t, int expected, int actual, const char* location) {
    if (expected == actual) {
        return;
    }

    fprintf(stderr, "ASSERTION FAILED expected %d actual %d at %s\n", expected, actual, location);
    longjmp(t->jump, 1);
}

void test_neqi_raw(struct test_context* t, int expected, int actual, const char* location) {
    if (expected != actual) {
        return;
    }

    fprintf(stderr, "ASSERTION FAILED expected not equal to %d at %s\n", expected, location);
    longjmp(t->jump, 1);
}

void test_ltf_raw(struct test_context* t, float left, float right, const char* location) {
    if (left < right) {
        return;
    }

    fprintf(stderr, "ASSERTION FAILED expected %f < %f at %s\n", left, right, location);
    longjmp(t->jump, 1);
}

void test_ltef_raw(struct test_context* t, float left, float right, const char* location) {
    if (left <= right) {
        return;
    }

    fprintf(stderr, "ASSERTION FAILED expected %f <= %f at %s\n", left, right, location);
    longjmp(t->jump, 1);
}

void test_gtf_raw(struct test_context* t, float left, float right, const char* location) {
    if (left > right) {
        return;
    }

    fprintf(stderr, "ASSERTION FAILED expected %f > %f at %s\n", left, right, location);
    longjmp(t->jump, 1);
}

void test_gtef_raw(struct test_context* t, float left, float right, const char* location) {
    if (left >= right) {
        return;
    }

    fprintf(stderr, "ASSERTION FAILED expected %f >= %f at %s\n", left, right, location);
    longjmp(t->jump, 1);
}

void test_near_equalf_raw(struct test_context* t, float expected, float actual, const char* location) {
    if (fabsf(expected - actual) < 0.000001f) {
        return;
    }

    fprintf(stderr, "ASSERTION FAILED expected %f ~= %f at %s\n", expected, actual, location);
    longjmp(t->jump, 1);
}

void test_vec2_equal_raw(struct test_context* t, struct Vector2* expected, struct Vector2* actual, const char* location) {
    if (vector2DistSqr(expected, actual) < 0.000001f) {
        return;
    }

    fprintf(
        stderr, 
        "ASSERTION FAILED expected <%f, %f> ~= <%f, %f> at %s\n", 
        expected->x, expected->y,
        actual->x, actual->y,
        location
    );
    longjmp(t->jump, 1);
}

void test_report_failures() {
    fprintf(stderr, "%d/%d tests passed\n", total_test_count - failed_test_count, total_test_count);

    for (int i = 0; i < failed_test_count && i < MAX_NAMED_FAILS; i += 1) {
        fprintf(stderr, "    TEST FAIL %s\n", failed_test_names[i]);
    }

    if (failed_test_count > MAX_NAMED_FAILS) {
        fprintf(stderr, "    and %d more", failed_test_count - MAX_NAMED_FAILS);
    }
}

static const char* current_test_scene_name;
static struct scene* current_test_scene;

void test_load_scene(const char* name) {
    if (current_test_scene_name && strcmp(name, current_test_scene_name) == 0) {
        return;
    }

    current_test_scene_name = name;

    scene_release(current_test_scene);
    current_test_scene = scene_load(name);
}

struct test_deferred_call_data {
    test_deferred_call callback;
    void* data;
};

#define MAX_DEFERRED_CALLBACKS  64

static struct test_deferred_call_data test_all_deferred_callbacks[MAX_DEFERRED_CALLBACKS];
static int deferred_callback_count = 0;

void test_defer_call(struct test_context* t, test_deferred_call callback, void* data) {
    if (deferred_callback_count >= MAX_DEFERRED_CALLBACKS) {
        fprintf(stderr, "test_defer_call overflow\n");
        longjmp(t->jump, 1);
    }

    test_all_deferred_callbacks[deferred_callback_count] = (struct test_deferred_call_data){
        callback,
        data
    };
    deferred_callback_count += 1;
}

void test_call_deferred() {
    while (deferred_callback_count > 0) {
        deferred_callback_count -= 1;
        struct test_deferred_call_data* curr = &test_all_deferred_callbacks[deferred_callback_count];
        curr->callback(curr->data);
    }
}