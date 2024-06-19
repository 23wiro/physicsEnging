// hours wasted here: 14
/* 

TODO:

    High Priority:

    - [x] fix balls not showing after exceeding initial capacity 
        (somehow the buffer is not being rezised properly even though the realloc is working fine. Problem doesnt appear untill initial capacity is reached)
        Nothing works

    Medium Priority:

    - [] fix the issue where the balls are not colliding with each other properly
        (no clue why)
    - [x] fix the issue where the balls slowly phase through the border
        (easy fix, just need to check if the ball is on the border and then disable gravity)    
    
    low priority:

    - [] visual representation of the border
    - [] end my suffering

*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

unsigned int VBO;
#define SLOP 0.0001
#define DAMPING 0.98
#define borderRadius 0.9f
#define NUM_SEGMENTS 20
#define INITIAL_CAPACITY 10
#define timeStep 0.01
float radius = 0.01f; // Normalized device coordinates range from -1 to 1
bool spacePressed = false; // prevents spawning multiple balls in one frame

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

        // Update the buffer size
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
    float x = p->position.x;
    float y = p->position.y;
    float vertices[NUM_SEGMENTS * 2];
    float angleStep = 2.0f * M_PI / numSegments;
    for (int i = 0; i < numSegments; i++) {
        float angle = i * angleStep;
        float dx = radius * cosf(angleStep);
        float dy = radius * sinf(angleStep);

        vertices[i] = x + dx;
        vertices[i + 1] = y + dy;
        glBufferSubData(GL_ARRAY_BUFFER, i * (NUM_SEGMENTS + 2) * 2 * sizeof(float), (NUM_SEGMENTS + 2) * 2 * sizeof(float), vertices);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

int borderCollision(centerPoint *p, float radius) {
    double distance = sqrt(p->position.x * p->position.x + p->position.y * p->position.y);
    double overlap = distance - (borderRadius - radius);

    if (overlap > 0) {
        double nx = p->position.x / distance;
        double ny = p->position.y / distance;

        // Separate the ball from the border
        p->position.x -= nx * (overlap + SLOP);
        p->position.y -= ny * (overlap + SLOP);

        // Apply impulse-based collision response
        double relativeVelocityX = p->velocity.x;
        double relativeVelocityY = p->velocity.y;
        double dotProduct = relativeVelocityX * nx + relativeVelocityY * ny;

        p->velocity.x -= (1 + DAMPING) * dotProduct * nx;
        p->velocity.y -= (1 + DAMPING) * dotProduct * ny;

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
        glBufferSubData(GL_ARRAY_BUFFER, i * (NUM_SEGMENTS + 2) * 2 * sizeof(float), (NUM_SEGMENTS + 2) * 2 * sizeof(float), vertices);  //if a circle is mising this is the problem (i think) hopyfully adding one to i in the offset will fix it
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void collisionDetection(pointArray *a, float radius) {
    for (int i = 0; i < a->size; i++) {
        for (int j = i + 1; j < a->size; j++) {
            double dx = a->points[i].position.x - a->points[j].position.x;
            double dy = a->points[i].position.y - a->points[j].position.y;
            double distance = sqrt(dx * dx + dy * dy);
            double overlap = 2 * radius - distance;

            if (overlap > 0) {
                double nx = dx / distance;
                double ny = dy / distance;

                // Separate the balls
                double correction = (overlap + SLOP) / 2;
                a->points[i].position.x += nx * correction;
                a->points[i].position.y += ny * correction;
                a->points[j].position.x -= nx * correction;
                a->points[j].position.y -= ny * correction;

                // Apply impulse-based collision response
                double relativeVelocityX = a->points[i].velocity.x - a->points[j].velocity.x;
                double relativeVelocityY = a->points[i].velocity.y - a->points[j].velocity.y;
                double dotProduct = relativeVelocityX * nx + relativeVelocityY * ny;

                a->points[i].velocity.x -= (1 + DAMPING) * dotProduct * nx;
                a->points[i].velocity.y -= (1 + DAMPING) * dotProduct * ny;
                a->points[j].velocity.x += (1 + DAMPING) * dotProduct * nx;
                a->points[j].velocity.y += (1 + DAMPING) * dotProduct * ny;
            }
        }
    }
}

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "   gl_Position = projection * vec4(aPos, 0.0, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "   FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
    "}\0";

void glfw_error_callback(int error, const char *description) {
    fprintf(stderr, "GLFW Error: %s\n", description);
}

void getMonitorResolution(int *width, int *height) {
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    *width = desktop.right;
    *height = desktop.bottom;
}

int main() {
    pointArray a;
    centerPoint boundryCenter;
    initPointArray(&a, INITIAL_CAPACITY);
    addPoint(&a, 0.0, 0.0, 1.0, 0.5); // Starting position and velocity

    int width, height;
    getMonitorResolution(&width, &height);

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(width, height, "Render Circle", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    glViewport(0, 0, width, height);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
        return -1;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
        return -1;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        return -1;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (NUM_SEGMENTS + 2) * 2 * sizeof(float) * a.capacity, NULL, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    float aspectRatio = (float)width / (float)height;
    float projection[16] = {
        1.0f / aspectRatio, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        float dx, dy;
        bool spaceCurrentlyPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        drawHollow(&boundryCenter, radius, NUM_SEGMENTS, VBO );
        if (spaceCurrentlyPressed && !spacePressed) {
            addPoint(&a, 0.0, 0.0, 1.0, 0.5);
            spacePressed = true;
        } else if (!spaceCurrentlyPressed) {
            spacePressed = false;
        }

        for (int i = 0; i < a.size; i++) {
            verlet(&a.points[i], timeStep, &dx, &dy);
            borderCollision(&a.points[i], radius);
        }

        collisionDetection(&a, radius);
        updateVertexData(&a, VBO, radius);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);

        glBindVertexArray(VAO);
        for (int i = 0; i < a.size; i++) {
            glDrawArrays(GL_TRIANGLE_FAN, i * (NUM_SEGMENTS + 2), NUM_SEGMENTS + 2);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();

    freePointArray(&a);
    return 0;
}