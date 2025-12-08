#ifndef __COLLISION_DYNAMIC_OBJECT_TEST_H__
#define __COLLISION_DYNAMIC_OBJECT_TEST_H__

#include "dynamic_object.h"
#include "../test/framework_test.h"

void test_verify_bb_calculator(struct test_context* t, void* data, bounding_box_calculator bb_calc, MinkowsiSum minkowsi_sum);

#endif