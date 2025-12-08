#include "ring_allocator.h"
#include "../test/framework_test.h"

#include <stddef.h>

bool ring_has_allocated_block(struct ring_allocator* allocator);

void test_ring_malloc(struct test_context* t) {
    struct ring_allocator allocator;
    ring_init(&allocator, 256);
    test_eqi(t, 256, ring_get_free_memory(&allocator));

    // basic allocation
    void* first = ring_malloc(&allocator, 8);
    test_neqi(t, (int)NULL, (int)first);
    test_eqi(t, true, ring_has_allocated_block(&allocator));
    test_eqi(t, 256 - 16, ring_get_free_memory(&allocator));
    ring_free(&allocator, first);
    test_eqi(t, false, ring_has_allocated_block(&allocator));
    test_eqi(t, 256, ring_get_free_memory(&allocator));

    // allocate all memory becuase
    // remaining memory is not enough
    // to make a block
    first = ring_malloc(&allocator, 248);
    test_neqi(t, (int)NULL, (int)first);
    test_eqi(t, true, ring_has_allocated_block(&allocator));
    test_eqi(t, 0, ring_get_free_memory(&allocator));
    test_eqi(t, (int)NULL, (int)allocator.next_free);
    test_eqi(t, (int)NULL, (int)allocator.last_free);
    ring_free(&allocator, first);
    test_eqi(t, false, ring_has_allocated_block(&allocator));
    test_eqi(t, 256, ring_get_free_memory(&allocator));
    test_neqi(t, (int)NULL, (int)allocator.next_free);
    test_neqi(t, (int)NULL, (int)allocator.last_free);

    first = ring_malloc(&allocator, 8);
    void* second = ring_malloc(&allocator, 8);
    test_eqi(t, 224, ring_get_free_memory(&allocator));
    ring_free(&allocator, first);
    ring_free(&allocator, second);
    test_eqi(t, 256, ring_get_free_memory(&allocator));

    first = ring_malloc(&allocator, 8);
    second = ring_malloc(&allocator, 8);
    test_eqi(t, 224, ring_get_free_memory(&allocator));
    ring_free(&allocator, second);
    ring_free(&allocator, first);
    test_eqi(t, 256, ring_get_free_memory(&allocator));

    first = ring_malloc(&allocator, 200);
    second = ring_malloc(&allocator, 8);
    void* third = ring_malloc(&allocator, 8);
    test_neqi(t, (int)NULL, (int)third);
    ring_free(&allocator, first);
    test_eqi(t, (int)NULL, (int)ring_malloc(&allocator, 256));
    first = ring_malloc(&allocator, 200);
    test_neqi(t, (int)NULL, (int)first);
    ring_free(&allocator, first);
    ring_free(&allocator, second);
    ring_free(&allocator, third);
    test_eqi(t, 256, ring_get_free_memory(&allocator));

    ring_destroy(&allocator);
}