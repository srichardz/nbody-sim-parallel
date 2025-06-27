#ifndef BH_SIM_UTILS_H
#define BH_SIM_UTILS_H

#define MAX(a, b) ((a)>(b)?(a):(b))         // return highest value
#define MIN(a, b) ((a)<(b)?(a):(b))         // return lowest value
#define CLAMP(a, b, c) (MIN(MAX(a,b),c))    // clamp a between b min and c max limits
// side effects like MAX(a++,b++) are possible

// prog
static const int WIDTH = 1280;
static const int HEIGTH = 720;
static int ZOOM = 250;
static const int FPS = 60;
static const double dt = 0.00001;

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

typedef struct Node Node;

struct Node {
    Vec2 center;
    double r; // shortest distance from center to side of rect, always square 
    double mass;
    Node* children[4];
    Body* obj;
    Vec2 center_of_mass;
};

typedef struct {
    Node* nodes;
    int index; // of last node inserted
    int size;
} Quadtree;

// sim core
#ifdef __cplusplus
extern "C" {
#endif
    void init_sim(Body* bodies, int* N, int* ts_done, Body* simulation_result[10000], int* flag, Node* root);
    void simulate(Body* bodies, int* N, double dt, int* last_done, Body* simulation_result[10000], int* flag, Node* root);
    // O(n^2) update scheme
    void brute_force_update(Body* bodies, int* N);
    // integrators
    void symplectic_euler(Body* obj, const Vec2* acc, const double dt);
    void explicit_euler(Body* obj, const Vec2* acc, const double dt);
    void runge_kutta_4(Body* obj, const Vec2* acc, const double dt);    // not implemented yet
    void leapfrog(Body* obj, const Vec2* acc, const double dt);         // not implemented yet

    void barnes_hut_update(Body* bodies, Node* root, int* N);
    void construct_tree(Body* bodies, Node* root, int* N, Quadtree* qt);
    void update_masses(Node* root);
    void force_calc(Node* root, Body* body, Vec2* acc);
#ifdef __cplusplus
}
#endif

#endif // BH_SIM_UTILS_H