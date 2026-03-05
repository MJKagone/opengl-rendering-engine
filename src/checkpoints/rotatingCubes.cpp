#define STB_IMAGE_IMPLEMENTATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ctime>
#include "stb_image.h"
#include "loadTexture.h"

const int WIDTH = 1280;
const int HEIGHT = 720;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

// Delta time
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Mouse parameters
float lastX = WIDTH / 2;
float lastY = HEIGHT / 2;
const float SENSITIVITY = 0.008f;
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float zoom = 45.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xPos, double yPos)
{
    //if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    //{
        if (firstMouse)
        {
            lastX = xPos;
            lastY = yPos;
            firstMouse = false;
        }

        float xOffset = xPos - lastX;
        float yOffset = lastY - yPos; // y-axis is reversed
        lastX = xPos;
        lastY = yPos;

        yaw += xOffset * SENSITIVITY;
        pitch += yOffset * SENSITIVITY;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(direction);

        glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);
        lastX = WIDTH/2;
        lastY = HEIGHT/2;
    //}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    zoom -= 2.0f * (float) yoffset;
    if (zoom < 1.0f) zoom = 1.0f;
    if (zoom > 45.0f) zoom = 45.0f;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
                                                                                                        const float cameraSpeed = 5.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;           if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;           if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));                                                                         if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));                                                                         if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos += cameraSpeed * cameraUp;              if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraUp;                                                                                                              }

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "GLFW Window", NULL, NULL);
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
    glViewport(0, 0, WIDTH, HEIGHT);

    // Handle resizing
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Capture the cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    // Define multiple cubes
    glm::vec3 cubePositions[] = {
            glm::vec3( 0.0f, 0.0f, -3.0f),
            glm::vec3( 2.0f, 5.0f, -2.2f),
            glm::vec3(-1.5f, -2.2f, -2.5f),
            glm::vec3(-3.8f, -2.0f, -3.1f),
            glm::vec3( 2.4f, -0.4f, -3.5f),
            glm::vec3(-1.7f, 3.0f, -4.0f),
            glm::vec3( 1.3f, -2.0f, -2.5f),
            glm::vec3( 1.5f, 2.0f, -2.5f),
            glm::vec3( 1.5f, 0.2f, -1.5f),
            glm::vec3(-1.3f, 1.0f, -1.5f)
    };

    // Configure shaders
    Shader objectShaders("vs_cubes.glsl", "fs_cubes.glsl");
    Shader lightShaders("vs_light.glsl", "fs_light.glsl");

    // Create and bind vertex buffer and array objects
    unsigned int VBO, VAO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
		(void*) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
		(void*) (6 * sizeof(float)));
	glEnableVertexAttribArray(2);

    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

	// Load textures
	unsigned int diffMap = loadTexture("containerDiff.png");
	unsigned int specMap = loadTexture("containerSpec.png");

	objectShaders.use();
	objectShaders.setInt("material.diffuse", 0);
	objectShaders.setInt("material.specular", 1);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
    	// Input
    	processInput(window);

        // Clear background
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Define delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

		// Define light(s)
		glm::vec3 dirLightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 dirLightPos(0.0f, 20.0f, 10.0f);
		glm::vec3 pointLightPositions[] = {
			glm::vec3(6.0f, -1.0f, -4.0f),
			glm::vec3(0.0f, 3.0f, -3.0f)
		};
		glm::vec3 pointLightColor = glm::vec3(255/255.0f, 197/255.0f, 143/255.0f);

        // Deploy shaders
        objectShaders.use();

        objectShaders.setFloat("material.shininess", 128.0f*0.4f);

		objectShaders.setVec3("dirLight.color", dirLightColor);
		objectShaders.setVec3("dirLight.direction", 0.0f, -20.0f, -10.0f);
        objectShaders.setVec3("dirLight.ambient", 0.3f, 0.3f, 0.3f);
        objectShaders.setVec3("dirLight.diffuse", 0.5f, 0.5f, 0.5f);
        objectShaders.setVec3("dirLight.specular", 1.0f, 1.0f, 1.0f);

		objectShaders.setVec3("pointLights[0].color", pointLightColor);
		objectShaders.setVec3("pointLights[0].position", pointLightPositions[0]);
		objectShaders.setVec3("pointLights[0].ambient", 0.3f, 0.3f, 0.3f);
		objectShaders.setVec3("pointLights[0].diffuse", 0.5f, 0.5f, 0.5f);
		objectShaders.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
		objectShaders.setFloat("pointLights[0].constant", 1.0f);
		objectShaders.setFloat("pointLights[0].linear", 0.09f);
		objectShaders.setFloat("pointLights[0].quadratic", 0.032f);
		objectShaders.setVec3("pointLights[1].color", pointLightColor);
        objectShaders.setVec3("pointLights[1].position", pointLightPositions[1]);
        objectShaders.setVec3("pointLights[1].ambient", 0.3f, 0.3f, 0.3f);
        objectShaders.setVec3("pointLights[1].diffuse", 0.5f, 0.5f, 0.5f);
        objectShaders.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        objectShaders.setFloat("pointLights[1].constant", 1.0f);
        objectShaders.setFloat("pointLights[1].linear", 0.09f);
        objectShaders.setFloat("pointLights[1].quadratic", 0.032f);		

        // Define camera and matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(zoom), (float) (WIDTH / HEIGHT), 0.1f, 100.0f);

		objectShaders.setMat4("projection", projection);
		objectShaders.setMat4("view", view);

        objectShaders.setVec3("viewPos", cameraPos);

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffMap);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specMap);
		// Loop for multiple cubes:
		glBindVertexArray(VAO);

		for (int i = 0; i < 10; i++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, cubePositions[i]);
			float angle = 20.0f * (i % 4 + 1);
			model = glm::rotate(model, glm::radians(angle) * (float) glfwGetTime(),
				glm::vec3(1.0f, 0.3f, 0.5f));
			objectShaders.setMat4("model", model);	
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

        // Lighting:

        lightShaders.use();
		lightShaders.setMat4("projection", projection);
		lightShaders.setMat4("view", view);
		lightShaders.setVec3("lightColor", 1.0f, 1.0f, 1.0f);

		glBindVertexArray(lightVAO);
        for (unsigned int i = 0; i < 2; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPositions[i]);
            model = glm::scale(model, glm::vec3(0.2f));
            lightShaders.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Check and call events and swap the buffers
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    // Terminate GLFW
    glfwTerminate();
    return 0;
}

