// hours wasted here: 14
/* 

TODO:

    High Priority:

    - [x] fix balls not showing after exceeding initial capacity 
        (somehow the buffer is not being rezised properly even though the realloc is working fine. Problem doesnt appear untill initial capacity is reached)
        Nothing works

    - [] fix the horrible jittering

    Medium Priority:

    - [x] fix the issue where the balls are not colliding with each other properly
        (no clue why)
    - [x] fix the issue where the balls slowly phase through the border
        (easy fix, just need to check if the ball is on the border and then disable gravity)    

    - [] rearange collision detection algorithm into chunks

    - [] parallelize the collision detection algorithm on the GPU (painful but worth it)
    
    low priority:

    - [] visual representation of the border
    - [] end my suffering
    - [] optimize updateVertexData function by updating the vertex data instead of creating a new array every time

*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include "common/common.h"

float SLOP = 0.0001;
float borderRadius = 0.9f;
int NUM_SEGMENTS = 20;
int INITIAL_CAPACITY = 20;
float timeStep = 0.01;
float M_PI = 3.14159265358979323846;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool spacePressed = false; // prevents spawning multiple balls in one frame

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

    unsigned int VAO, VBO;
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