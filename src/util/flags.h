#ifndef __UTIL_FLAGS_H__
#define __UTIL_FLAGS_H__

#define HAS_FLAG(value, flag) (((value) & (flag)) != 0)
#define SET_FLAG(value, flag) (value) |= (flag)
#define CLEAR_FLAG(value, flag) (value) &= ~(flag)

#endif