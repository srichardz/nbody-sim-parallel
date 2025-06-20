#ifndef BH_SIM_UTILS_H
#define BH_SIM_UTILS_H

// math
typedef struct {
    double x;
    double y;
} Vec2;

// objs
typedef struct {
    Vec2 pos;
    Vec2 v;
    double mass;
} Body;

#endif // BH_SIM_UTILS_H