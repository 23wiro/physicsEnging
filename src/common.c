#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include "common/common.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NUM_SEGMENTS 10
#define borderRadius 0.9f
#define SLOP 0.0001

unsigned int VBO;
float radius = 0.01f;



void initPointArray(pointArray *a, int initialSize) {
    a->points = (centerPoint *)malloc(initialSize * sizeof(centerPoint));
    a->size = 0;
    a->capacity = initialSize;
}

void freePointArray(pointArray *a) {
    free(a->points);
    a->points = NULL;
    a->size = 0;
    a->capacity = 0;
}

list*** initChunkArray(int divisionX, int divisionY) {
    list ***chunkArray = (list ***)malloc(divisionX * sizeof(list **));
    for (int i = 0; i < divisionX; i++) {
        chunkArray[i] = (list **)malloc(divisionY * sizeof(list *));
        for (int j = 0; j < divisionY; j++) {
            chunkArray[i][j] = (list *)malloc(sizeof(list)); // Assuming list is a struct
            chunkArray[i][j]->data = -1; // Assuming 'data' is a pointer within 'list'
        }
    }
    return chunkArray;
}

void freeList(list *head) {
    list *tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

void freeChunkArray(int divisionX, int divisionY, list ***chunkArray) {
    for (int i = 0; i < divisionX; i++) {
        for (int j = 0; j < divisionY; j++) {
            // Free each list node here, assuming a function freeList exists
            freeList(chunkArray[i][j]);
        }
    }
    free(chunkArray);
}

// Function to add a new node to the list
list* addToList(list *head, centerPoint *p) {
    list *newNode = (list*)malloc(sizeof(list));
    if (newNode == NULL) {
        fprintf(stderr, "Epic malloc failure\n");
        return head; // Early return on failure
    }

    newNode->data = p->indexInPointArray;
    newNode->next = head;
    return newNode; // Return the new head of the list
}

void addToChunk(centerPoint *p, list ***chunkArray) {
    int xIndex = (int)(p->gridPosition.x);
    int yIndex = (int)(p->gridPosition.y);
    chunkArray[xIndex][yIndex] = addToList(chunkArray[xIndex][yIndex], p);
}
 
// Function to get data from the list by index
int getDataOfIndex(list *head, int index, int *data) {
    if (head == NULL) {
        printf("List is empty\n");
        return 0; // Indicate failure or empty list
    }

    list *current = head;
    int currentIndex = 0;

    while (current != NULL) {
        if (currentIndex == index) {
            *data = current->data;
            return 1; // Success
        }
        current = current->next;
        currentIndex++;
    }

    return 0; // Indicate that index was out of bounds
}

int getIndexOfData(list *head, int data, int *index) {
    if (head == NULL) {
        printf("List is empty\n");
        return 0; // Indicate failure or empty list
    }

    list *current = head;
    int currentIndex = 0;

    while (current != NULL) {
        if (current->data == data) {
            return currentIndex; // Success
        }
        current = current->next;
        currentIndex++;
    }
    return 0;
}

// Function to set data in the list by index
int setDataOfIndex(list *head, int index, int data) {
    list *current = head;
    int currentIndex = 0;

    while (current != NULL && currentIndex < index) {
        current = current->next;
        currentIndex++;
    }

    if (current == NULL) {
        printf("Index out of bounds\n");
        return 0;
    }

    current->data = data;
    return 1;
}

// Function to remove a node from the list by index
void popFromList(list **head, int index) {
    if (*head == NULL) {
        printf("List is empty\n");
        return;
    }

    list *current = *head;
    if (index == 0) {
        *head = current->next;
        free(current);
        return;
    }

    int currentIndex = 0;
    while (current != NULL && currentIndex < index - 1) {
        current = current->next;
        currentIndex++;
    }

    if (current == NULL || current->next == NULL) {
        printf("Index out of bounds\n");
        return;
    }

    list *ptrToNext = current->next->next;
    free(current->next);
    current->next = ptrToNext;
}

void addPoint(pointArray *a, double x, double y, double vx, double vy) {
    if (a->size >= a->capacity) {
        a->capacity *= 2;
        a->points = (centerPoint *)realloc(a->points, a->capacity * sizeof(centerPoint));
        if (a->points == NULL) {
            fprintf(stderr, "Epic realloc failure\n");
            return;
        }
        printf("Resized array to %d\n", a->capacity);

    }
    printf("Added point at %d\n", a->size);
    centerPoint *p = &a->points[a->size++];
    p->position = (vector2){x, y};
    p->velocity = (vector2){vx, vy};
    p->acceleration = (vector2){0.0, 0.0};
}

void circleGen(centerPoint *p, float radius, int numSegments, float *vertices) {
    float angleStep = 2.0f * M_PI / numSegments;
    vertices[0] = p->position.x;
    vertices[1] = p->position.y;
    for (int i = 1; i <= numSegments + 1; i++) {
        float angle = i * angleStep;
        vertices[2 * i] = p->position.x + radius * cos(angle);
        vertices[2 * i + 1] = p->position.y + radius * sin(angle);
    }
}

void drawHollow(centerPoint *p, float radius, int numSegments, unsigned int VBO) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    float vertices[NUM_SEGMENTS * 2];
    float angleStep = 2.0f * M_PI / numSegments;
    for (int i = 0; i < numSegments; i++) {
        float angle = i * angleStep;
        float dx = radius * cosf(angleStep);
        float dy = radius * sinf(angleStep);

        vertices[i] = p->position.x + dx;
        vertices[i + 1] = p->position.y + dy;
        glBufferSubData(GL_ARRAY_BUFFER, i * (NUM_SEGMENTS + 2) * 2 * sizeof(float), (NUM_SEGMENTS + 2) * 2 * sizeof(float), vertices);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

int borderCollision(centerPoint *p, float radius) {
    float distance = sqrt(p->position.x * p->position.x + p->position.y * p->position.y);
    if (distance >= borderRadius - radius) {
        // Calculate the normal direction (outward from the center)
        float nx = p->position.x / distance;
        float ny = p->position.y / distance;
        
        // Reverse velocity along the normal direction
        float dotProduct = p->velocity.x * nx + p->velocity.y * ny;
        float damping = 0.95; // Example damping factor, adjust as needed
        p->velocity.x -= 2 * dotProduct * nx * damping;
        p->velocity.y -= 2 * dotProduct * ny * damping;
        
        // Reposition the ball just inside the boundary
        p->position.x = (borderRadius - radius) * nx;
        p->position.y = (borderRadius - radius) * ny;
        
        return 1;
    }
    return 0;
}

void gravity(centerPoint *p) {
    const double G = -9.81;
    p->acceleration = (vector2){0.0, G};
}

void verlet(centerPoint *p, double dt, int subSteps, float cellWidth, float cellHeight, list ***chunkArray) {
    float frictionCoeficient = 0.1;
    float frictionForce = frictionCoeficient * p->velocity.y;

    double subDt = dt / subSteps; // Calculate sub-step duration
    for (int step = 0; step < subSteps; ++step) {
        float dx, dy;
        dx = p->velocity.x * subDt + 0.5 * p->acceleration.x * subDt * subDt;
        dy = p->velocity.y * subDt + 0.5 * p->acceleration.y * subDt * subDt;
        p->position.x += dx;
        p->position.y += dy;

        if (!borderCollision(p, radius)) {
            gravity(p);
        }

        int xIndex = (int)(p->position.x) / cellWidth;
        int yIndex = (int)(p->position.y) / cellHeight;

        p->gridPosition = (vector2){xIndex, yIndex};

        addToChunk(p, chunkArray);

        p->velocity.x += p->acceleration.x * subDt;
        p->velocity.y += p->acceleration.y * subDt;

        double velocityThreshold = 0.001;
        if(fabs(p->velocity.x) < velocityThreshold && fabs(p->velocity.y) < velocityThreshold) {
            p->velocity.x = 0.0;
            p->velocity.y = 0.0;
        }

        if (p->isColliding) {

        }
    }

    p->velocity.x += p->acceleration.x * dt;
    p->velocity.y += p->acceleration.y * dt;

    double velocityThreshold = 0.001;
    if(fabs(p->velocity.x) < velocityThreshold && fabs(p->velocity.y) < velocityThreshold) {
        p->velocity.x = 0.0;
        p->velocity.y = 0.0;
    }

}

void updateVertexData(pointArray *a, unsigned int VBO, float radius) {
    int totalSize = a->size * (NUM_SEGMENTS + 2) * 2 * sizeof(float);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Resize the buffer to ensure it can hold all vertices
    glBufferData(GL_ARRAY_BUFFER, totalSize, NULL, GL_DYNAMIC_DRAW);

    for (int i = 0; i < a->size; ++i) {
        float vertices[(NUM_SEGMENTS + 2) * 2];
        circleGen(&a->points[i], radius, NUM_SEGMENTS, vertices);
        // Update the buffer with vertex data for each point
        glBufferSubData(GL_ARRAY_BUFFER, i * (NUM_SEGMENTS + 2) * 2 * sizeof(float), (NUM_SEGMENTS + 2) * 2 * sizeof(float), vertices);
        // Draw the point
        glDrawArrays(GL_TRIANGLE_FAN, i * (NUM_SEGMENTS + 2), NUM_SEGMENTS + 2);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}



void collisionDetection(pointArray *a, float radius, list ***chunkArray, vector2 *gridSize) {
    if (!a || !gridSize || !chunkArray) {
        fprintf(stderr, "Error: Null pointer passed to collisionDetection.\n");
        return; // Early return to avoid dereferencing null pointers
    }
    if (!a->points || a->size <= 0) {
        fprintf(stderr, "Error: Invalid pointArray structure.\n");
        return; // Early return if pointArray is invalid
    }

    const double damping = 0.9; // Damping factor to reduce jittering
    const double slop = SLOP; // Small threshold for allowable overlap

    int divisionX = gridSize->x;
    int divisionY = gridSize->y;

    for (int i; i < divisionX; i++) {
        for (int j; j < divisionY; j++) {
            list *current = chunkArray[i][j];
            int listSize;
            while(current != NULL) {
                listSize++;
                current = current->next;
            }

            if (listSize < 2){
                continue;
            }

            for(int k; k <= listSize; k++) {
                for (int l; l < listSize; l++) {
                    int index1, index2;
                    getDataOfIndex(chunkArray[i][j], k, &index1);
                    getDataOfIndex(chunkArray[i][j], l, &index2);

                    if (index1 == index2) {
                        continue;
                    }

                    centerPoint *p = &a->points[index1];
                    centerPoint *q = &a->points[index2];

                    double dx = p->position.x - q->position.x;
                    double dy = p->position.y - q->position.y;
                    double distance = sqrt(dx * dx + dy * dy);
                    double overlap = 2 * radius - distance;

                    if (overlap > slop) {
                        double nx = dx / distance;
                        double ny = dy / distance;
                        p->position.x += nx * (overlap - slop) / 2;
                        p->position.y += ny * (overlap - slop) / 2;
                        q->position.x -= nx * (overlap - slop) / 2;
                        q->position.y -= ny * (overlap - slop) / 2;

                        // Calculate new velocities
                        double vx = p->velocity.x - q->velocity.x;
                        double vy = p->velocity.y - q->velocity.y;
                        double dotProduct = vx * nx + vy * ny;

                        // Apply the collision response with damping
                        p->velocity.x = (p->velocity.x - dotProduct * nx) * damping;
                        p->velocity.y = (p->velocity.y - dotProduct * ny) * damping;
                        q->velocity.x = (q->velocity.x + dotProduct * nx) * damping;
                        q->velocity.y = (q->velocity.y + dotProduct * ny) * damping;
                    }
                }
            }
        }
    }
}