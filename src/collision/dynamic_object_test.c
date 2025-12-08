
#include "dynamic_object_test.h"

#include <stdio.h>
#include "../math/mathf.h"
#include "../math/quaternion.h"

void test_verify_bb_calculator(struct test_context* t, void* data, bounding_box_calculator bb_calc, MinkowsiSum minkowsi_sum) {
    for (int angle = 0; angle < 360; angle += 45) {
        struct Vector2 rotation;
        vector2ComplexFromAngle(DEG_TO_RAD(angle), &rotation);

        fprintf(stderr, "  testing angle = %d\n", angle);

        struct Box3D bb_output;
        bb_calc(data, &rotation, &bb_output);

        for (int i = 0; i < 6; i += 1) {
            struct Vector3 direction = gZeroVec;
            int axis = i >> 1;

            VECTOR3_AS_ARRAY(&direction)[axis] = (i & 1) ? 1.0f : -1.0f;

            struct Quaternion dirRotation;
            quatAxisAngle(&gUp, -DEG_TO_RAD(angle), &dirRotation);
            quatMultVector(&dirRotation, &direction, &direction);

            struct Vector3 minkowsi_result;
            minkowsi_sum(data, &direction, &minkowsi_result);

            quatConjugate(&dirRotation, &dirRotation);
            quatMultVector(&dirRotation, &minkowsi_result, &minkowsi_result);

            fprintf(stderr, "    testing axis=%d dir=%d\n", axis, (i & 1) ? 1 : -1);
            test_near_equalf(
                t,
                VECTOR3_AS_ARRAY(&minkowsi_result)[axis],
                (i & 1) ? VECTOR3_AS_ARRAY(&bb_output.max)[axis] : VECTOR3_AS_ARRAY(&bb_output.min)[axis]
            );
        }    
    }
}