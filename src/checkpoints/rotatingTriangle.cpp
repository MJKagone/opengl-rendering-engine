#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(800, 600, "GLFW Window", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    // Define viewport
    glViewport(0, 0, 800, 600);

    // Handle resizing
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Define vertices
    float vertices[] = {
	 // positions        colors
	-0.4f, -0.2f, 0.0f, 1.0f, 0.0f, 0.0f,
         0.5f, 0.3f, 0.0f, 0.0f, 1.0f, 0.0f,
         0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    // Configure shaders
    Shader shaders("vs.glsl", "fs.glsl");

    // Create and bind vertex buffer and array objects
    unsigned int VBO, VAO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Input
        processInput(window);

        // Clear background
        glClearColor(0.80f, 0.80f, 0.80f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

	// Deploy shaders
        shaders.use();

	// Define transformation matrix
	glm::mat4 transf = glm::mat4(1.0f);
	transf = glm::translate(transf, glm::vec3(0.0f, 0.0f, 0.0f));
	transf = glm::rotate(transf, (float) glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
    	unsigned int transfLoc = glGetUniformLocation(shaders.ID, "transform");
    	glUniformMatrix4fv(transfLoc, 1, GL_FALSE, glm::value_ptr(transf));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Check and call events and swap the buffers
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    // Terminate GLFW
    glfwTerminate();
    return 0;
}

