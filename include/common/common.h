#ifndef COMMON_H
#define COMMON_H

typedef struct {
    double x, y;
} vector2;

typedef struct {
    vector2 position;
    vector2 velocity;
    vector2 acceleration;
    vector2 gridPosition;
    int indexInPointArray;
} centerPoint;

typedef struct list {
    int data;
    struct list *next;
} list;

typedef struct {
    centerPoint *points;
    int size;
    int capacity;
} pointArray;

extern unsigned int VBO;
extern float radius;


void initPointArray(pointArray *a, int initialSize);

void freePointArray(pointArray *a);

list* addToList(list *head, centerPoint *p);

list*** initChunkArray(int divisionX, int divisionY);

void freeChunkArray(int divisionX, int divisionY, list ***chunkArray);

void addToChunk(centerPoint *p, list ***chunkArray);

int getDataOfIndex(list *head, int index, int *data);

int setDataOfIndex(list *head, int index, int data);

void popFromList(list **head, int index);

void addPoint(pointArray *a, double x, double y, double vx, double vy);

void circleGen(centerPoint *p, float radius, int numSegments, float *vertices);

void drawHollow(centerPoint *p, float radius, int numSegments, unsigned int VBO);

void verlet(centerPoint *p, double dt, int subSteps, float cellWidth, float cellHeight, list ***chunkArray);

int borderCollision(centerPoint *p, float radius);

void collisionDetection(pointArray *a, float radius, list ***chunkArray, vector2 *gridSize);

void updateVertexData(pointArray *a, unsigned int VBO, float radius);

#endif // common.h