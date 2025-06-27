// bh_sim_utils.c
#include "bh_sim_utils.h"
#include <stdlib.h>
#include "math.h"
#include "time.h"
#include <stdio.h>

void init_sim(Body* bodies, int* N, int* last_done, Body* simulation_result[10000], int* flag, Node* root) {

    for(int i=0;i<(*N);i++) {
        double u1 = rand() / ((double)RAND_MAX);
        double u2 = rand() / ((double)RAND_MAX);

        double r = 1*sqrt(u1);
        double th = 2.0*PI*u2;

        double x = r*cos(th);
        double y = r*sin(th);

        double R = 1.0;
        double M = (1.0*(*N))*(r*r)/1.0;
        double v_mag = sqrt(M/r);

        bodies[i].pos = (Vec2){x, y};
        bodies[i].v = (Vec2){-1*v_mag*sin(th), v_mag*cos(th)};
        //bodies[i].v = (Vec2){0.0, 0.0};

        bodies[i].mass = 1.0;
    }
//    bodies[0].pos = (Vec2){-50.0,0.0};
//    bodies[1].pos = (Vec2){50.0,0.0};

//    bodies[0].v = (Vec2){0.0,sqrt(200.0)/2};
//    bodies[1].v = (Vec2){0.0,-1*sqrt(200.0)/2};
//    //disable printf("%f\n", sqrt(pow(bodies[1].v.x-bodies[0].v.x, 2)+pow(bodies[1].v.y-bodies[0].v.y, 2)));

//    bodies[0].mass = 1000000.0;
//    bodies[1].mass = 1000000.0;
    
    // large center point mass
    // bodies[0].pos.x = 0.0;
    // bodies[0].pos.y = 0.0;
    // bodies[0].v.x = 0.0;
    // bodies[0].v.y = 0.0;
    // bodies[0].mass = 2.0;

    simulate(bodies, N, dt, last_done, simulation_result, flag, root);
}

void simulate(Body* bodies, int* N, double dt, int* last_done, Body* simulation_result[10000], int* flag, Node* root) {
    
    double elapsed = 0.0;

    while(*flag)
    {

        // brute_force_update(bodies, N);
        
        barnes_hut_update(bodies, root, N);

        // barnes-hut end

        // saving timestep to render
        elapsed += dt;
        if(elapsed>1/FPS) {
            (*last_done)++;
            simulation_result[CLAMP(*last_done, 0, 10000-1)] = bodies;
            elapsed = 0.0;
        }
    }
}

// Integrator schemes for brute force update
void symplectic_euler(Body* obj, const Vec2* acc, const double dt) {
    // v(t_i+1) = v(t_i)+a(t_i)*dt
    // x(t_i+1) = x(t_i)+v(t_i)*dt
    obj->v = (Vec2){obj->v.x+acc->x*dt,obj->v.y+acc->y*dt};
    obj->pos = (Vec2){obj->pos.x+obj->v.x*dt,obj->pos.y+obj->v.y*dt};
}

void explicit_euler(Body* obj, const Vec2* acc, const double dt) {
    // v(t_i+1) = v(t_i)+a(t_i)*dt
    // x(t_i+1) = x(t_i)+v(t_i)*dt
    obj->pos = (Vec2){obj->pos.x+obj->v.x*dt,obj->pos.y+obj->v.y*dt};
    obj->v = (Vec2){obj->v.x+acc->x*dt,obj->v.y+acc->y*dt};
}

void runge_kutta_4(Body* obj, const Vec2* acc, const double dt) {}

void leapfrog(Body* obj, const Vec2* acc, const double dt) {}

// O(n^2) brute force update scheme
void brute_force_update(Body* bodies, int* N) {

    for(int i=0;i<(*N);i++) {
        Vec2 acc = {0.0,0.0};

        for(int j=0;j<(*N);j++) {
            if(i!=j){
                double dx,dy,dist;
                    
                dx = bodies[j].pos.x-bodies[i].pos.x;
                dy = bodies[j].pos.y-bodies[i].pos.y;
                    
                double rmag = sqrt(dx*dx+dy*dy);
                double epsilon = 0.01;

                double ax = bodies[j].mass*dx/pow(rmag*rmag+epsilon*epsilon,1.5);
                double ay = bodies[j].mass*dy/pow(rmag*rmag+epsilon*epsilon,1.5);

                acc.x += ax;
                acc.y += ay;
            }
        }
        symplectic_euler(&bodies[i], &acc, dt);

    }
};

// O(nlogn) barnes_hut optimization
void barnes_hut_update(Body* bodies, Node* root, int* N) {
    Quadtree qt;
    construct_tree(bodies, root, N, &qt);
    update_masses(&qt.nodes[0]);
    for(int i=0;i<(*N);i++) {
        Vec2 acc = (Vec2){0.0,0.0};
        force_calc(&qt.nodes[0], &bodies[i], &acc);
        symplectic_euler(&bodies[i], &acc, dt);
    }

    free(qt.nodes);
}

void insert_body(Body* body, Node* root, Quadtree* qt) {
    // 1. If node x does not contain a body, put the new body b here. 
    if(root->obj==NULL && root->children[0]==NULL) {
        root->obj = body;
    } // 2. If node x is an internal node, recursively insert the body b in the appropriate quadrant.
    else if (root->children[0] != NULL) { // if null, node is external (leaf)

        // recursively insert the body b in the appropriate quadrant
        // 2.1. appropriate quadrant
        // (n, w)
        // (0 0) = 0 north east
        // (0 1) = 1 north west
        // (1 0) = 2 south east
        // (1 1) = 3 south west

        int quadrant = 2*(body->pos.y > root->center.y ? 0 : 1)+(body->pos.x > root->center.x ? 1 : 0);
        insert_body(body, root->children[quadrant], qt);
    }
    else { // at this point the node is guaranteed to be external (leaf)
        // subdivide quadrant -> initialize children
        // recursively insert b and c into appropriate quadrant(s).
        
        int quadrant_b = 2*(root->obj->pos.y > root->center.y ? 0 : 1)+(root->obj->pos.x > root->center.x ? 1 : 0);
        int quadrant_c = 2*(body->pos.y > root->center.y ? 0 : 1)+(body->pos.x > root->center.x ? 1 : 0);

        Body* obj_buffer = root->obj;
        root->obj = NULL;

        for(int i=0;i<4;i++) {
            Node buf;
            buf.obj = NULL;
            for(int i=0;i<4;i++) {
                buf.children[i] = NULL;
            }
            buf.center = (Vec2){root->center.x+((((i&1)==0)?-1:1)*(root->r/2)),root->center.y+(((i>>1)==0?1:-1)*(root->r/2))};
            buf.center_of_mass = (Vec2){0.0,0.0};
            buf.r = root->r/2;
            buf.mass = 0.0;

//            printf("%d\n", qt->index);
            if (qt->index >= qt->size) {
                fprintf(stderr, "Quadtree overflow\n");
                exit(1);
            }
            qt->nodes[qt->index] = buf;
            root->children[i] = &(qt->nodes[qt->index]);
            (qt->index)++;
        }

        insert_body(obj_buffer, root->children[quadrant_b], qt);
        insert_body(body, root->children[quadrant_c], qt);
    }
}

// barnes-hut
void construct_tree(Body* bodies, Node* root, int* N, Quadtree* qt) {
    
    (*qt).nodes = (Node*)malloc(sizeof(Node)*(8*(*N))); // 4 times N should be a safe limit
    (*qt).index = 0;
    (*qt).size = 8*(*N);
    // init root of quadtree
    root->center = (Vec2){0.0,0.0};
    root->obj = NULL;

    double furthest = 0.0; 
    for(int i=0;i<(*N);i++) {
        Vec2 d_ = (Vec2){root->center.x-bodies[i].pos.x,root->center.y-bodies[i].pos.y};
        double dist = sqrt(d_.x*d_.x+d_.y*d_.y);
        if(dist>furthest) {
            furthest = dist;
        }
    }
    root->r = furthest;
    
    root->mass = 0.0;
    for(int i=0;i<4;i++){ root->children[i]=NULL; }
    root->center_of_mass = (Vec2){0.0, 0.0};
    // loop through all bodies
    //disable printf("Starting update loop\n");
    for(int i=0;i<(*N);i++){
        insert_body(&bodies[i], root, qt);
    }
    //disable printf("Finished update loop\n");

};

// marked
void update_masses(Node* root) {
    if (root->obj!=NULL && root->children[0]==NULL) {
        root->mass = root->obj->mass;
        root->center_of_mass = root->obj->pos;
        return;
    }
    root->mass = 0.0;
    Vec2 weighted_sum = {0.0, 0.0};

    for(int i=0;i<4;i++) {
        if (root->children[i]) {
            update_masses(root->children[i]);
            root->mass += root->children[i]->mass;
            weighted_sum.x+=root->children[i]->center_of_mass.x*root->children[i]->mass;
            weighted_sum.x+=root->children[i]->center_of_mass.y*root->children[i]->mass;
        }
    }

    if (root->mass > 0.0) {
        root->center_of_mass.x = weighted_sum.x / root->mass;
        root->center_of_mass.y = weighted_sum.y / root->mass;
    }
}

void force_calc(Node* root, Body* body, Vec2* acc) {
    double force = 0.0;
    if (root->obj!=NULL && root->children[0]==NULL && root->obj!=body) {
        Vec2 d_ = (Vec2){root->center_of_mass.x-body->pos.x, root->center_of_mass.y-body->pos.y};
        double d = sqrt(d_.x*d_.x+d_.y*d_.y);
        double epsilon = 0.01;

        double ax = root->mass*d_.x/pow(d*d+epsilon*epsilon,1.5);
        double ay = root->mass*d_.y/pow(d*d+epsilon*epsilon,1.5);

        (*acc).x += ax;
        (*acc).y += ay;
    } else if (root->children[0]!=NULL) {
        Vec2 d_ = (Vec2){root->center_of_mass.x-body->pos.x, root->center_of_mass.y-body->pos.y};
        double d = sqrt(d_.x*d_.x+d_.y*d_.y);
        double theta = 0.5;
        if(root->r/d<theta) {
            double epsilon = 0.01;

            double ax = root->mass*d_.x/pow(d*d+epsilon*epsilon,1.5);
            double ay = root->mass*d_.y/pow(d*d+epsilon*epsilon,1.5);

            (*acc).x += ax;
            (*acc).y += ay;
        } else {
            for(int i=0;i<4;i++) {
                if(root->children[i]) {
                    force_calc(root->children[i], body, acc);
                }
            }
        }
    }
}

//clock_t start = clock();
//clock_t end = clock();
//float seconds = (float)(end - start) / CLOCKS_PER_SEC;
////disable printf("%f seconds\n", seconds);
