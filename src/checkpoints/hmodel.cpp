#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ctime>
// #include "loadTexture.h"
#include "Model.h"

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
	    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;           if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;           if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));                                                                         if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));                                                                         if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos += cameraSpeed * cameraUp;
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraUp;
}

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

	// Enable depth testing and blending
    glEnable(GL_DEPTH_TEST);
	
	// Define shaders
	Shader shaders("vs.glsl", "fs.glsl");

	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

	// Load model(s)
	Model object1("hmodel-assets/base.obj");
	Model object2("hmodel-assets/arm.obj");
	Model object3("hmodel-assets/finger.obj");

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
		shaders.use();
		shaders.setFloat("shininess", 140.0f);
		shaders.setVec3("dirLight.color", dirLightColor);
        shaders.setVec3("dirLight.direction", 0.0f, -20.0f, -10.0f);
        shaders.setVec3("dirLight.ambient", 0.3f, 0.3f, 0.3f);
        shaders.setVec3("dirLight.diffuse", 0.5f, 0.5f, 0.5f);
        shaders.setVec3("dirLight.specular", 1.0f, 1.0f, 1.0f);

		// Define camera and matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(zoom), (float) (WIDTH / HEIGHT), 0.1f, 100.0f);
        shaders.setMat4("projection", projection);
        shaders.setMat4("view", view);
        shaders.setVec3("viewPos", cameraPos);

        // Render the loaded model(s)
		std::vector<glm::mat4> mvpStack;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(10.0f) * (float) glfwGetTime(),
					glm::vec3(0.0f, 1.0f, 0.0f)); // rotates constantly
		mvpStack.push_back(model);
		model = glm::scale(model, glm::vec3(0.45f, 0.50f, 0.45f));
		shaders.setMat4("model", model);
        object1.Draw(shaders); // base

		model = mvpStack.back();
		model = glm::translate(model, glm::vec3(0.0f, 0.4f, 0.0f));
		model = glm::rotate(model, glm::radians(40.0f) * sin((float) glfwGetTime()),
					glm::vec3(1.0f, 0.0f, 0.0f)); // alternates between -40 and 40 degrees
		mvpStack.push_back(model);
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
		shaders.setMat4("model", model);
		object2.Draw(shaders); // arm

		model = mvpStack.back();
		model = glm::translate(model, glm::vec3(0.0f, 0.265f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f) * (float) glfwGetTime(),
					glm::vec3(0.0f, 0.0f, 1.0f)); // rotates constantly
		model = glm::scale(model, glm::vec3(0.3f, 0.5f, 0.3f));
		shaders.setMat4("model", model);
		object3.Draw(shaders); // finger

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
