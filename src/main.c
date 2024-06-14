#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float radius = 0.1; // Normalized device coordinates range from -1 to 1
#define NUM_SEGMENTS 100

typedef struct {
    double x, y;
} vector2;

typedef struct {
    vector2 position;
    vector2 velocity;
    vector2 acceleration;
} centerPoint;

void initCenterpoint(centerPoint *p, double x, double y, double vx, double vy) {
    p->position = (vector2){x, y};
    p->velocity = (vector2){vx, vy};
    p->acceleration = (vector2){0.0, 0.0};
}

void gravity(centerPoint *p) {
    const double G = -9.81;
    p->acceleration = (vector2){0.0, G};
}

void verlet(centerPoint *p, double dt) {
    vector2 temp = p->position;

    p->position.x += p->velocity.x * dt + 0.5 * p->acceleration.x * dt * dt;
    p->position.y += p->velocity.y * dt + 0.5 * p->acceleration.y * dt * dt;

    gravity(p);

    p->velocity.x += (p->acceleration.x * dt);
    p->velocity.y += (p->acceleration.y * dt);
}

// Vertex shader source code
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "   gl_Position = projection * vec4(aPos, 0.0, 1.0);\n"
    "}\0";

// Circle vertices generation
void circleGen(float centerX, float centerY, float radius, int numSegments, float *points) {
    float angleStep = 2 * M_PI / numSegments;

    // Center vertex
    points[0] = centerX;
    points[1] = centerY;

    for (int i = 1; i <= numSegments + 1; i++) {
        float angle = (i - 1) * angleStep;
        points[i * 2] = centerX + radius * cos(angle);
        points[i * 2 + 1] = centerY + radius * sin(angle);
    }
}

// Fragment shader source code
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "   FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
    "}\0";

// Callback function for handling GLFW errors
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
    centerPoint p;
    initCenterpoint(&p, 0.0, 0.0, 1.0, 1.0); // Starting position and velocity
    int width, height;

    // Set the error callback
    glfwSetErrorCallback(glfw_error_callback);

    // Get the monitor resolution
    getMonitorResolution(&width, &height);

    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // Set the OpenGL version (3.3) and profile (core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow *window = glfwCreateWindow(width, height, "Render Circle", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Load OpenGL function pointers using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    // Set the viewport
    glViewport(0, 0, width, height);

    // Build and compile the shader program
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check for vertex shader compile errors
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

    // Check for fragment shader compile errors
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

    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        return -1;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Set up the vertex array object (VAO) and vertex buffer object (VBO)
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    float vertices[(NUM_SEGMENTS + 2) * 2]; // *2 if each vertex consists of 2 floats (x and y)
    float centerX = p.position.x;
    float centerY = p.position.y;
    circleGen(centerX, centerY, radius, NUM_SEGMENTS, vertices);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Unbind the VAO
    glBindVertexArray(0);

    // Create an orthographic projection matrix
    float aspectRatio = (float)width / (float)height;
    float projection[16] = {
        1.0f / aspectRatio, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Get the location of the projection matrix uniform
    int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Update the particle position using Verlet integration
        verlet(&p, 0.01); // Small time step

        // Update the circle vertices based on the new position
        circleGen(p.position.x, p.position.y, radius, NUM_SEGMENTS, vertices);

        // Update the vertex buffer data
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        // Render the circle
        glUseProgram(shaderProgram);

        // Set the projection matrix uniform
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, NUM_SEGMENTS + 2);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Clean up and exit
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}