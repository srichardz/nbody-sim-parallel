// bh_sim_utils.c
#include "bh_sim_utils.h"
#include "stdio.h"
#include "math.h"

void init_sim(Body* bodies, int* last_done, Body* simulation_result[10000], int* flag) {

    for(int i=0;i<N;i++) {
        float test[2];
        test[0] = 0.0+100.0*cos(i*2*PI/N);
        test[1] = 0.0+100.0*sin(i*2*PI/N);
        bodies[i].pos = (Vec2){test[0], test[1]};
        double mag_ = sqrt(test[0]*test[0]+test[1]*test[1]);
        bodies[i].v = (Vec2){test[1]/mag_, -1*test[0]/mag_};
//        bodies[i].v = (Vec2){0.0, 0.0};
        bodies[i].mass = 10000000.0;
    }

//    for(int i=0;i<N;i++) {
//        Body buf;
//        buf.pos = (Vec2){(float)i*10.0,(float)i*10.0};
//        buf.mass = 100000.0;
//        buf.v = (Vec2){0.0, 0.0};
//        bodies[i] = buf;
//    }

    simulate(bodies, 0.0001, last_done, simulation_result, flag);

}

void simulate(Body* bodies, double dt, int* last_done, Body* simulation_result[10000], int* flag) {

    double elapsed = 0.0;

    while(*flag)
    {
        for(int i=0;i<N;i++) {
            Vec2 acc = {0.0,0.0};

            for(int j=0;j<N;j++) {
                if (i!=j){
                    double dx,dy,dist;
                    dx = bodies[j].pos.x-bodies[i].pos.x;
                    dy = bodies[j].pos.y-bodies[i].pos.y;
                    dist = dx*dx+dy*dy;
                    double invDist3 = 1.0 / (dist * sqrt(dist)) + 0.000001;
                    // Acceleration contribution: G * m_j * (r_ij) / |r_ij|^3
                    acc.x += bodies[j].mass * dx * invDist3;
                    acc.y += bodies[j].mass * dy * invDist3;
//                    printf("%f %f %f %f\n", sqrt(acc.x*acc.x+acc.y*acc.y), invDist3, dx, dy);
                }
            }

            Vec2 v1 = {bodies[i].v.x+acc.x*dt,bodies[i].v.y+acc.y*dt};
            bodies[i].pos = (Vec2){bodies[i].pos.x+v1.x*dt,bodies[i].pos.y+v1.y*dt};
        }
        elapsed += dt;
        if(elapsed>1/FPS) {
            (*last_done)++;
            simulation_result[CLAMP(*last_done, 0, 10000-1)] = bodies;
            elapsed = 0.0;
        }
    }
}