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

#define NUM_SEGMENTS 20
#define borderRadius 0.9f
#define SLOP 0.0001

// Define these variables in common.c
unsigned int VBO;
float radius = 0.01f; // Move the definition from main.c to here


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

void addPoint(pointArray *a, double x, double y, double vx, double vy) {
    if (a->size >= a->capacity) {
        a->capacity *= 2;
        centerPoint *newPoints = (centerPoint *)realloc(a->points, a->capacity * sizeof(centerPoint));
        if (newPoints == NULL) {
            fprintf(stderr, "Epic realloc failure\n");
            return;
        }
        a->points = newPoints;
        printf("Resized array to %d\n", a->capacity);

        // Update VBO buffer size
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, (NUM_SEGMENTS + 2) * 2 * sizeof(float) * a->capacity, NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
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
        p->velocity.x -= 2 * dotProduct * nx;
        p->velocity.y -= 2 * dotProduct * ny;
        
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

void verlet(centerPoint *p, double dt, float *dx, float *dy) {
    *dx = p->velocity.x * dt + 0.5 * p->acceleration.x * dt * dt;
    *dy = p->velocity.y * dt + 0.5 * p->acceleration.y * dt * dt;
    p->position.x += *dx;
    p->position.y += *dy;

    if (!borderCollision(p, radius)) {
        gravity(p);
    }

    p->velocity.x += p->acceleration.x * dt;
    p->velocity.y += p->acceleration.y * dt;
}

void updateVertexData(pointArray *a, unsigned int VBO, float radius) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    for (int i = 0; i < a->size; ++i) {
        float vertices[(NUM_SEGMENTS + 2) * 2];
        circleGen(&a->points[i], radius, NUM_SEGMENTS, vertices);
        glBufferSubData(GL_ARRAY_BUFFER, i * (NUM_SEGMENTS + 2) * 2 * sizeof(float), (NUM_SEGMENTS + 2) * 2 * sizeof(float), vertices);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void collisionDetection(pointArray *a, float radius) {
    const double damping = 0.98; // Damping factor to reduce jittering
    const double slop = SLOP; // Small threshold for allowable overlap

    for (int i = 0; i < a->size; i++) {
        for (int j = i + 1; j < a->size; j++) {
            double dx = a->points[i].position.x - a->points[j].position.x;
            double dy = a->points[i].position.y - a->points[j].position.y;
            double distance = sqrt(dx * dx + dy * dy);
            double overlap = 2 * radius - distance;

            if (overlap > slop) {
                // Separate the balls
                double nx = dx / distance;
                double ny = dy / distance;
                a->points[i].position.x += nx * (overlap - slop) / 2;
                a->points[i].position.y += ny * (overlap - slop) / 2;
                a->points[j].position.x -= nx * (overlap - slop) / 2;
                a->points[j].position.y -= ny * (overlap - slop) / 2;

                // Calculate new velocities
                double vx = a->points[i].velocity.x - a->points[j].velocity.x;
                double vy = a->points[i].velocity.y - a->points[j].velocity.y;
                double dotProduct = vx * nx + vy * ny;

                // Apply the collision response with damping
                a->points[i].velocity.x = (a->points[i].velocity.x - dotProduct * nx) * damping;
                a->points[i].velocity.y = (a->points[i].velocity.y - dotProduct * ny) * damping;
                a->points[j].velocity.x = (a->points[j].velocity.x + dotProduct * nx) * damping;
                a->points[j].velocity.y = (a->points[j].velocity.y + dotProduct * ny) * damping;
            }
        }
    }
}