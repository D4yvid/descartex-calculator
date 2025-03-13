#ifndef UTIL_VEC_H
#define UTIL_VEC_H

#include <math.h>

typedef struct v2 {
    int x, y;
} v2;

typedef struct v2f {
    float x, y;
} v2f;

typedef struct v3 {
    int x, y, z;
} v3;

typedef struct v3f {
    float x, y, z;
} v3f;

#define v3f(...) ((struct v3f) { __VA_ARGS__ })
#define v3(...) ((struct v3) { __VA_ARGS__ })
#define v2f(...) ((struct v2f) { __VA_ARGS__ })
#define v2(...) ((struct v2) { __VA_ARGS__ })

#define v2_add(a, b) ((struct v2) { .x = (a).x + (b).x, .y = (a).y + (b).y })
#define v2f_add(a, b) ((struct v2f) { .x = (a).x + (b).x, .y = (a).y + (b).y })
#define v3_add(a, b) ((struct v3) { .x = (a).x + (b).x, .y = (a).y + (b).y, .z = (a).z + (b).z })
#define v3f_add(a, b) ((struct v3f) { .x = (a).x + (b).x, .y = (a).y + (b).y, .z = (a).z + (b).z })

#define v2_sub(a, b) ((struct v2) { .x = (a).x - (b).x, .y = (a).y - (b).y })
#define v2f_sub(a, b) ((struct v2f) { .x = (a).x - (b).x, .y = (a).y - (b).y })
#define v3_sub(a, b) ((struct v3) { .x = (a).x - (b).x, .y = (a).y - (b).y, .z = (a).z - (b).z })
#define v3f_sub(a, b) ((struct v3f) { .x = (a).x - (b).x, .y = (a).y - (b).y, .z = (a).z - (b).z })

#define v2_mul(a, b) ((struct v2) { .x = (a).x * (b).x, .y = (a).y * (b).y })
#define v2f_mul(a, b) ((struct v2f) { .x = (a).x * (b).x, .y = (a).y * (b).y })
#define v3_mul(a, b) ((struct v3) { .x = (a).x * (b).x, .y = (a).y * (b).y, .z = (a).z * (b).z })
#define v3f_mul(a, b) ((struct v3f) { .x = (a).x * (b).x, .y = (a).y * (b).y, .z = (a).z * (b).z })

#define v2_div(a, b) ((struct v2) { .x = (a).x / (b).x, .y = (a).y / (b).y })
#define v2f_div(a, b) ((struct v2f) { .x = (a).x / (b).x, .y = (a).y / (b).y })
#define v3_div(a, b) ((struct v3) { .x = (a).x / (b).x, .y = (a).y / (b).y, .z = (a).z / (b).z })
#define v3f_div(a, b) ((struct v3f) { .x = (a).x / (b).x, .y = (a).y / (b).y, .z = (a).z / (b).z })

#endif /** UTIL_VEC_H */
