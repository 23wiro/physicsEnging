#ifndef COMMON_H
#define COMMON_H

typedef struct {
    double x, y;
} vector2;

typedef struct {
    vector2 position;
    vector2 velocity;
    vector2 acceleration;
} centerPoint;

typedef struct {
    centerPoint *points;
    int size;
    int capacity;
} pointArray;

extern unsigned int VBO;
extern float radius;


void initPointArray(pointArray *a, int capacity);

void freePointArray(pointArray *a);

void addPoint(pointArray *a, double x, double y, double vx, double vy);

void circleGen(centerPoint *p, float radius, int numSegments, float *vertices);

void drawHollow(centerPoint *p, float radius, int numSegments, unsigned int VBO);

void verlet(centerPoint *p, double timeStep, float *dx, float *dy);

int borderCollision(centerPoint *p, float radius);

void collisionDetection(pointArray *a, float radius);

void updateVertexData(pointArray *a, unsigned int VBO, float radius);

#endif // common.h