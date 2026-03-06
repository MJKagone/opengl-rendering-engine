#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include "Camera.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ctime>
// #include "loadTexture.h"
#include "Model_robust.h"

const int WIDTH = 1280;
const int HEIGHT = 720;

float lastX = WIDTH / 2;
float lastY = HEIGHT / 2;

bool shadersActive = true;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Camera cam = Camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xPos, double yPos)
{
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

    cam.processMouse(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cam.processScroll((float) yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        shadersActive = !shadersActive;
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        std::cout << "Camera position: (" << cam.pos.x << ", " << cam.pos.y << ", " << cam.pos.z << ")\n";
    }

}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {cam.processKeyboard(Camera::FORWARD, deltaTime);}
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {cam.processKeyboard(Camera::BACKWARD, deltaTime);}
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {cam.processKeyboard(Camera::LEFT, deltaTime);}
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {cam.processKeyboard(Camera::RIGHT, deltaTime);}
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {cam.processKeyboard(Camera::UP, deltaTime);}
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {cam.processKeyboard(Camera::DOWN, deltaTime);}
}

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_DEPTH_BITS, 32);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "GLFW", NULL, NULL);
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
    glfwSetKeyCallback(window, key_callback);

	// Enable depth testing and blending
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
	
	// Define shaders
	Shader phongShaders("shaders/vertex/vs.glsl", "shaders/fragment/fs_phong.glsl");
    Shader constantShaders("shaders/vertex/vs.glsl", "shaders/fragment/fs_constant.glsl");
	Shader lightSourceShaders("shaders/vertex/vs_lightSource.glsl", "shaders/fragment/fs_lightSource.glsl");
    Shader skyboxShaders("shaders/vertex/vs_skybox.glsl", "shaders/fragment/fs_skybox.glsl");
    Shader shadowShaders("shaders/vertex/vs_shadows.glsl", "shaders/fragment/fs_shadows.glsl");
    Shader debugShaders("shaders/vertex/vs_debug.glsl", "shaders/fragment/fs_debug.glsl");

    //stbi_set_flip_vertically_on_load(true);

	// Load models
	Model object1("assets/models/modern-bedroom/source/Bedroom.fbx");

    // Debug line for directional light
    unsigned int lineVBO, lineVAO;
    glGenBuffers(1, &lineVBO);
    glGenVertexArrays(1, &lineVAO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

	// Light cube vertices
	float lightVertices[] = {
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
	};
	unsigned int lightVBO, lightVAO;
	glGenBuffers(1, &lightVBO);
    glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lightVertices), lightVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
	glEnableVertexAttribArray(0);

    // Generate shadow maps
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    const int SHADOW_WIDTH = 1024;
    const int SHADOW_HEIGHT = 1024;
    
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);


    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // debugShaders.use();
    // debugShaders.setInt("depthMap", 0);

    //////////////////////
    // MAIN RENDER LOOP //
    //////////////////////
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
        glm::vec3 dirLightPos = glm::vec3(2*13.0f, 2*17.0f, 2*(-30.0f));
        glm::vec3 pointLightPositions[] = {
            glm::vec3(-8.0f, 4.2f, -14.0f),
            glm::vec3(6.9f, 4.2f, -14.0f),
			glm::vec3(13.8f, 3.2f, 0.58f),
			glm::vec3(4.0f-0.7, 4.5f, 1.5f+0.25)
        };

		glm::vec3 pointLightColor1 = glm::vec3(240/255.0f, 150/255.0f, 80/255.0f);
		glm::vec3 pointLightColor2 = glm::vec3(240/255.0f, 150/255.0f, 80/255.0f);
		glm::vec3 pointLightColor3 = glm::vec3(100/255.0f, 100/255.0f, 255/255.0f);
		glm::vec3 pointLightColor4 = glm::vec3(255/255.0f, 255/255.0f, 0/255.0f);

		phongShaders.use();
		phongShaders.setFloat("shininess", 140.0f);
		phongShaders.setVec3("dirLight.color", dirLightColor);
        glm::vec3 lightDirection = glm::vec3(0.0f, 0.0f, 0.0f) - dirLightPos;
        phongShaders.setVec3("dirLight.direction", lightDirection);
        phongShaders.setVec3("dirLight.ambient", 0.3f, 0.3f, 0.3f);
        phongShaders.setVec3("dirLight.diffuse", 0.5f, 0.5f, 0.5f);
        phongShaders.setVec3("dirLight.specular", 1.0f, 1.0f, 1.0f);

        phongShaders.setVec3("pointLights[0].color", pointLightColor1);
        phongShaders.setVec3("pointLights[0].position", pointLightPositions[0]);
        phongShaders.setVec3("pointLights[0].ambient", 0.3f, 0.3f, 0.3f);
        phongShaders.setVec3("pointLights[0].diffuse", 0.5f, 0.5f, 0.5f);
        phongShaders.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        phongShaders.setFloat("pointLights[0].constant", 1.0f);
        phongShaders.setFloat("pointLights[0].linear", 0.07f);
        phongShaders.setFloat("pointLights[0].quadratic", 0.017f);

        phongShaders.setVec3("pointLights[1].color", pointLightColor2);
        phongShaders.setVec3("pointLights[1].position", pointLightPositions[1]);
        phongShaders.setVec3("pointLights[1].ambient", 0.3f, 0.3f, 0.3f);
        phongShaders.setVec3("pointLights[1].diffuse", 0.5f, 0.5f, 0.5f);
        phongShaders.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        phongShaders.setFloat("pointLights[1].constant", 1.0f);
        phongShaders.setFloat("pointLights[1].linear", 0.07f);
        phongShaders.setFloat("pointLights[1].quadratic", 0.017f);
        
        // Laptop light
		// phongShaders.setVec3("pointLights[2].color", pointLightColor3);
        // phongShaders.setVec3("pointLights[2].position", pointLightPositions[2]);
        // phongShaders.setVec3("pointLights[2].ambient", 0.15f, 0.15f, 0.15f);
        // phongShaders.setVec3("pointLights[2].diffuse", 0.3f, 0.3f, 0.3f);
        // phongShaders.setVec3("pointLights[2].specular", 0.1f, 0.1f, 0.1f);
        // phongShaders.setFloat("pointLights[2].constant", 1.0f);
        // phongShaders.setFloat("pointLights[2].linear", 0.22f);
        // phongShaders.setFloat("pointLights[2].quadratic", 0.20f);

        // shaders.setVec3("pointLights[3].color", pointLightColor4);
        // shaders.setVec3("pointLights[3].position", pointLightPositions[3]);
        // shaders.setVec3("pointLights[3].ambient", 0.3f, 0.3f, 0.3f);
        // shaders.setVec3("pointLights[3].diffuse", 0.5f, 0.5f, 0.5f);
        // shaders.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
        // shaders.setFloat("pointLights[3].constant", 1.0f);
        // shaders.setFloat("pointLights[3].linear", 0.07f);
        // shaders.setFloat("pointLights[3].quadratic", 0.017f);

        /////////////////////////////////////////////////////////
        // 1. PASS: RENDER SHADOW MAP FROM LIGHTS' PERSPECTIVE //
        // NOTE: currently only for directional light          //
        /////////////////////////////////////////////////////////
        if (shadersActive) {
            float near_plane = 10.0f;
            float far_plane = 100.0f;

            glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
            glm::mat4 lightView = glm::lookAt(dirLightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;

            glCullFace(GL_FRONT);

            shadowShaders.use();
            shadowShaders.setMat4("lightSpaceMatrix", lightSpaceMatrix);

            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.03f, 0.03f, 0.03f));
            // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            shadowShaders.setMat4("model", model);
            object1.Draw(shadowShaders);

            // model = glm::mat4(1.0f);
            // model = glm::translate(model, glm::vec3(-0.5f, -1.0f, -4.0f));
            // model = glm::scale(model, glm::vec3(0.02f, 0.02f, 0.02f));
            // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            // shadowShaders.setMat4("model", model);
            // object2.Draw(shadowShaders);

            glCullFace(GL_BACK);

            // Render debug quad

            // glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // glViewport(0, 0, WIDTH, HEIGHT);
            // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // debugShaders.use();
            // debugShaders.setInt("depthMap", 0);
            // glActiveTexture(GL_TEXTURE0);
            // glBindTexture(GL_TEXTURE_2D, depthMap);
            // renderQuad();

            // Render scene with loaded models
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, WIDTH, HEIGHT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            phongShaders.use();
            phongShaders.setInt("shadowMap", 9); // 1 -> 9, Gemini suggestion
            phongShaders.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_2D, depthMap);
        }

        //////////////////////////////////////////////////////////
        // 2. PASS: RENDER SCENE USING THE GENERATED SHADOW MAP //
        //////////////////////////////////////////////////////////
        glm::mat4 view = glm::lookAt(cam.pos, cam.pos + cam.front, cam.worldUp);
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(cam.fov), (float) (WIDTH / HEIGHT), 0.1f, 100.0f);

        if (shadersActive) {
            phongShaders.setMat4("projection", projection);
            phongShaders.setMat4("view", view);

            phongShaders.setVec3("viewPos", cam.pos);
        }
        else {
            constantShaders.use();
            constantShaders.setMat4("projection", projection);
            constantShaders.setMat4("view", view);
            constantShaders.setVec3("viewPos", cam.pos);
        }

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.03f, 0.03f, 0.03f));
        // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        if (shadersActive) {
            phongShaders.setMat4("model", model);
            phongShaders.setFloat("transparency", 1.0f);
            object1.Draw(phongShaders);
        }
        else {
            constantShaders.setMat4("model", model);
            constantShaders.setFloat("transparency", 1.0f);
            object1.Draw(constantShaders);
        }

		// model = glm::mat4(1.0f);
		// model = glm::translate(model, glm::vec3(-0.5f, -1.0f, -4.0f));
        // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// model = glm::scale(model, glm::vec3(0.02f, 0.02f, 0.02f));
		// shaders.setMat4("model", model);
		// shaders.setFloat("transparency", 1.0f);
		// object2.Draw(shaders);

        ////////////////////////////////////////////
		// RENDER LIGHT SOURCES FOR VISUALIZATION //
        ////////////////////////////////////////////
		lightSourceShaders.use();
		lightSourceShaders.setMat4("projection", projection);
		lightSourceShaders.setMat4("view", view);

        // Point lights
		// lightSourceShaders.setVec3("lightColor", pointLightColor1);
		// model = glm::mat4(1.0f);
		// model = glm::translate(model, pointLightPositions[0]);
		// model = glm::scale(model, glm::vec3(0.05f));
		// lightSourceShaders.setMat4("model", model);
		// glBindVertexArray(lightVAO);
		// glDrawArrays(GL_TRIANGLES, 0, 36);

		// lightSourceShaders.setVec3("lightColor", pointLightColor2);
		// model = glm::mat4(1.0f);
		// model = glm::translate(model, pointLightPositions[1]);
		// model = glm::scale(model, glm::vec3(0.05f));
        // // model = glm::rotate(model, glm::radians(35.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// lightSourceShaders.setMat4("model", model);
		// glBindVertexArray(lightVAO);
		// glDrawArrays(GL_TRIANGLES, 0, 36);

        // lightSourceShaders.setVec3("lightColor", pointLightColor3);
        // model = glm::mat4(1.0f);
        // model = glm::translate(model, pointLightPositions[2]);
        // model = glm::scale(model, glm::vec3(0.2f));
        // lightSourceShaders.setMat4("model", model);
        // glBindVertexArray(lightVAO);
        // glDrawArrays(GL_TRIANGLES, 0, 36);

        // lightSourceShaders.setVec3("lightColor", pointLightColor4);
        // model = glm::mat4(1.0f);
        // model = glm::translate(model, pointLightPositions[3]);
        // model = glm::scale(model, glm::vec3(0.2f));
        // lightSourceShaders.setMat4("model", model);
        // glBindVertexArray(lightVAO);
        // glDrawArrays(GL_TRIANGLES, 0, 36);

        // Directional light
        lightSourceShaders.setVec3("lightColor", dirLightColor);
        model = glm::mat4(1.0f);
        model = glm::translate(model, dirLightPos); 
        // model = glm::scale(model, glm::vec3(1.0f));
        lightSourceShaders.setMat4("model", model);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Debug line
        glm::vec3 lineStart = dirLightPos;
        glm::vec3 lineEnd = dirLightPos + glm::normalize(lightDirection) * 5.0f; 

        float lineVertices[] = {
            lineStart.x, lineStart.y, lineStart.z,
            lineEnd.x, lineEnd.y, lineEnd.z
        };

        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);

        lightSourceShaders.use();
        lightSourceShaders.setMat4("projection", projection);
        lightSourceShaders.setMat4("view", view);
        lightSourceShaders.setMat4("model", glm::mat4(1.0f)); 
        lightSourceShaders.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.0f)); // Yellow line

        glDrawArrays(GL_LINES, 0, 2);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}