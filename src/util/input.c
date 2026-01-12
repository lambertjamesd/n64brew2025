#include "input.h"

#define MAX_RANGE       80
#define DEADZONE        8
#define FIXED_BIT_SIZE  23

#define SCALAR          (DEADZONE * (1 << FIXED_BIT_SIZE) / (MAX_RANGE - DEADZONE))

int input_handle_deadzone(int input) {
    if (input > DEADZONE) {
        return ((input - DEADZONE) * SCALAR) >> FIXED_BIT_SIZE;
    } else if (input < -DEADZONE) {
        return ((input + DEADZONE) * SCALAR) >> FIXED_BIT_SIZE;
    } else {
        return 0;
    }
}