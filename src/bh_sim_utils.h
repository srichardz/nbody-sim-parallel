#ifndef BH_SIM_UTILS_H
#define BH_SIM_UTILS_H

#define MAX(a, b) ((a)>(b)?(a):(b))         // return highest value
#define MIN(a, b) ((a)<(b)?(a):(b))         // return lowest value
#define CLAMP(a, b, c) (MIN(MAX(a,b),c))    // clamp a between b min and c max limits
// side effects like MAX(a++,b++) are possible

// prog
static const int WIDTH = 1280;
static const int HEIGTH = 720;
static const int N = 10;
static const int FPS = 60;

// math

static const double PI = 3.14159265358979323846;

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

// sim core
#ifdef __cplusplus
extern "C" {
#endif
    void init_sim(Body* bodies, int* ts_done, Body* simulation_result[10000], int* flag);
#ifdef __cplusplus
}
#endif

#endif // BH_SIM_UTILS_H