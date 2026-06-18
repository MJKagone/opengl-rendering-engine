#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include "Camera.hpp"
#include "Shader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ctime>
// #include "loadTexture.h"
#include "Model.hpp"

GLuint loadEquirectangularMap(const char* path);
glm::vec3 srgbToLinear(glm::vec3 srgb);

bool firstMouse = true;
bool skyboxToggle = true;
bool fpsToggle = true;
bool vSyncToggle = true;

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int SHADOW_WIDTH = 1024;
const int SHADOW_HEIGHT = 1024;
const int NUM_POINT_LIGHTS = 4;

int frameCount = 0;

const float NEAR_PLANE = 0.5f;
const float FAR_PLANE = 100.0f;
const float ROTATION_SPEED = 275.0f;

float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
float exposure = 1.0f;
float fpsTimer = 0.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;



Camera cam = Camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

enum ShaderType {
    PHONG,
    CONSTANT,
    DEPTH
};
int shaderType = PHONG;


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
        if (shaderType >= CONSTANT) {shaderType = PHONG;} else {shaderType++;}
    }
    if (key == GLFW_KEY_B && action == GLFW_PRESS)
    {
        if (skyboxToggle == false) {skyboxToggle = true;} else {skyboxToggle = false;}
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        std::cout << "Camera position: (" << cam.pos.x << ", " << cam.pos.y << ", " << cam.pos.z << ")\n";
    }
    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    {
        exposure += 0.5f;
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    {
        exposure -= 0.5f;
        if (exposure < 0.5f) {exposure = 0.5f;}
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        if (fpsToggle == false) {fpsToggle = true;} else {fpsToggle = false;}
    }
    if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        if (vSyncToggle == true)
        {
            glfwSwapInterval(0);
            vSyncToggle = false;
        }
        else
        {
            glfwSwapInterval(1);
            vSyncToggle = true;
        }
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
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    // glfwWindowHint(GLFW_DEPTH_BITS, 32);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "GLFW", NULL, NULL);
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
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

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
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_FRAMEBUFFER_SRGB);
	
	// Define shaders
    std::cout << "Compiling shaders...\n";
	Shader phongShaders("shaders/vertex/vs.glsl", "shaders/fragment/fs_phong.glsl");
    Shader constantShaders("shaders/vertex/vs.glsl", "shaders/fragment/fs_constant.glsl");
	Shader lightSourceShaders("shaders/vertex/vs_lightSource.glsl", "shaders/fragment/fs_lightSource.glsl");
    Shader skyboxShaders("shaders/vertex/vs_skybox.glsl", "shaders/fragment/fs_skybox.glsl");
    Shader dirShadowShaders("shaders/vertex/vs_dirShadows.glsl", "shaders/fragment/fs_dirShadows.glsl");
    Shader pointShadowShaders("shaders/vertex/vs_pointShadows.glsl", "shaders/fragment/fs_pointShadows.glsl", "shaders/geometry/gs_pointShadows.glsl");
    Shader debugShaders("shaders/vertex/vs_debug.glsl", "shaders/fragment/fs_debug.glsl");
    Shader depthShaders("shaders/vertex/vs.glsl", "shaders/fragment/fs_depth.glsl");
    Shader quadShaders("shaders/vertex/vs_quad.glsl", "shaders/fragment/fs_quad.glsl");

    // Pre-allocate uniform strings
    std::string shadowMatrixNames[6];
    for (int j = 0; j < 6; ++j) {
        shadowMatrixNames[j] = "shadowMatrices[" + std::to_string(j) + "]";
    }

    std::string shadowCubemapNames[NUM_POINT_LIGHTS];
    for (int i = 0; i < NUM_POINT_LIGHTS; ++i) {
        shadowCubemapNames[i] = "shadowCubemaps[" + std::to_string(i) + "]";
    }

    //stbi_set_flip_vertically_on_load(true);

	// Load models
    std::cout << "Loading models...\n";
	Model scene("assets/models/modern-bedroom/source/Bedroom.fbx");
    Model lamp("assets/models/ceiling-fan(2)/source/ceiling_fan.fbx");

    // Load skybox from equirectangular image
    GLuint skybox = loadEquirectangularMap("assets/skybox/spacebox_8x.png");

    // Debug line for directional light
    GLuint lineVBO, lineVAO;
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
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
	};
	GLuint lightVBO, lightVAO;
	glGenBuffers(1, &lightVBO);
    glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lightVertices), lightVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
	glEnableVertexAttribArray(0);

    // Final render target: screen-filling HDR quad
    float quadVertices[] = {  
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
    };	
    GLuint quadFBO, quadVBO, quadVAO;
    glGenFramebuffers(1, &quadFBO);

    GLuint quadTexture;
    glGenTextures(1, &quadTexture);
    glBindTexture(GL_TEXTURE_2D, quadTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, quadFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, quadTexture, 0);

    GLuint rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenBuffers(1, &quadVBO);
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Directional light shadow map FBO and texture
    GLuint shadowMapFBO;
    glGenFramebuffers(1, &shadowMapFBO);
    
    GLuint shadowMap;
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Point light shadow cubemap FBOs and textures
    GLuint shadowCubemapFBO[NUM_POINT_LIGHTS];
    GLuint shadowCubemap[NUM_POINT_LIGHTS];

    for (int i = 0; i < NUM_POINT_LIGHTS; ++i)
    {
        glGenFramebuffers(1, &shadowCubemapFBO[i]);
        glGenTextures(1, &shadowCubemap[i]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap[i]);
        
        for (int j = 0; j < 6; ++j)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowCubemapFBO[i]);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowCubemap[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // debugShaders.use();
    // debugShaders.setInt("shadowMap", 0);

    // Define model transformations
    glm::vec3 scenePos = glm::vec3(0.0f, -2.0f, 0.0f);
    glm::vec3 sceneScale = glm::vec3(0.03f);
    glm::vec3 lampPos = glm::vec3(4.0f, 16.2f, -3.0f);
    glm::vec3 lampScale = glm::vec3(0.075f);


    // Define light(s)
    glm::vec3 dirLightColor = glm::vec3(255.0f/255.0f, 255.0f/255.0f, 240.0f/255.0f);
    float dirLightIntensity = 5.0f;
    glm::vec3 dirLightPos = glm::vec3(2*35.0f, 2*18.0f, 2*(-0.0f));
    glm::vec3 pointLightPositions[] = {
        glm::vec3(-8.05f, 4.35f, -14.0f), // left bedside lamp
        glm::vec3(6.8f, 4.35f, -14.0f), // right bedside lamp
        glm::vec3(4.0f, 12.2f, -3.0f), // ceiling fan light
        glm::vec3(15.0f, 3.5f, 0.07f) // laptop light
    };

    glm::vec3 pointLightColor1 = glm::vec3(240.0f/255.0f, 180.0f/255.0f, 150.0f/255.0f);
    float pointLight1Intensity = 20.0f;
    glm::vec3 pointLightColor2 = glm::vec3(240.0f/255.0f, 180.0f/255.0f, 150.0f/255.0f);
    float pointLight2Intensity = 20.0f;
    glm::vec3 pointLightColor3 = glm::vec3(255.0f/255.0f, 200.0f/255.0f, 180.0f/255.0f);
    float pointLight3Intensity = 100.0f;
    glm::vec3 pointLightColor4 = glm::vec3(100.0f/255.0f, 100.0f/255.0f, 200.0f/255.0f);
    float pointLight4Intensity = 5.0f;

    phongShaders.use();
    phongShaders.setInt("numPointLights", NUM_POINT_LIGHTS);
    phongShaders.setFloat("far_plane", 25.0f);
    phongShaders.setFloat("globalAmbient", 0.05f);

    // Initialize all samplerCube uniforms to prevent type conflicts on texture unit 0
    for (int i = 0; i < 10; i++) { 
        phongShaders.setInt("shadowCubemaps[" + std::to_string(i) + "]", 10 + i);
    }
    
    phongShaders.setVec3("dirLight.color", srgbToLinear(dirLightColor) * dirLightIntensity);
    glm::vec3 lightDirection = glm::vec3(0.0f, 0.0f, 0.0f) - dirLightPos;
    phongShaders.setVec3("dirLight.direction", lightDirection);

    phongShaders.setVec3("pointLights[0].color", srgbToLinear(pointLightColor1) * pointLight1Intensity);
    phongShaders.setVec3("pointLights[0].position", pointLightPositions[0]);
    
    phongShaders.setVec3("pointLights[1].color", srgbToLinear(pointLightColor2) * pointLight2Intensity);
    phongShaders.setVec3("pointLights[1].position", pointLightPositions[1]);
    
    phongShaders.setVec3("pointLights[2].color", srgbToLinear(pointLightColor3) * pointLight3Intensity);
    phongShaders.setVec3("pointLights[2].position", pointLightPositions[2]);

    // Laptop light
    phongShaders.setVec3("pointLights[3].color", srgbToLinear(pointLightColor4) * pointLight4Intensity);
    phongShaders.setVec3("pointLights[3].position", pointLightPositions[3]);

    skyboxShaders.use();
    skyboxShaders.setInt("skybox", 0);

    //////////////////////
    // MAIN RENDER LOOP //
    //////////////////////
	while (!glfwWindowShouldClose(window))
	{
        // Input
		processInput(window);
        quadShaders.use();
        quadShaders.setFloat("exposure", exposure);
        
        // Clear background
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Define delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // FPS counter
        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 0.2f && fpsToggle)
        {
            std::cout << "\rFPS: " << round(frameCount / fpsTimer) << "   " << std::flush;
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        /////////////////////////////////////////////////////////
        // 1. PASS: RENDER SHADOW MAP FROM LIGHTS' PERSPECTIVE //
        /////////////////////////////////////////////////////////
        if (shaderType == PHONG) {

            // 1.1: Render directional light shadow map
            glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, NEAR_PLANE, FAR_PLANE);
            glm::mat4 lightView = glm::lookAt(dirLightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;
            
            glCullFace(GL_FRONT);
            
            dirShadowShaders.use();
            dirShadowShaders.setMat4("dirLightSpaceMatrix", lightSpaceMatrix);
            
            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);
            
            // Render scene with loaded models
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, scenePos);
            model = glm::scale(model, sceneScale);
            dirShadowShaders.setMat4("model", model);
            scene.Draw(dirShadowShaders);

            model =  glm::mat4(1.0f);
            model = glm::translate(model, lampPos);
            model = glm::scale(model, lampScale);
            model = glm::rotate(model, glm::radians(ROTATION_SPEED) * (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
            dirShadowShaders.setMat4("model", model);
            lamp.Draw(dirShadowShaders);

            phongShaders.use();
            phongShaders.setInt("shadowMap", 9); // 1 -> 9, Gemini suggestion
            phongShaders.setMat4("dirLightSpaceMatrix", lightSpaceMatrix);
            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_2D, shadowMap);
            // glBindFramebuffer(GL_FRAMEBUFFER, 0); // unnecessary?

            // 1.2: Render point light shadow cubemaps
            for (int i = 0; i < NUM_POINT_LIGHTS; i++) {

                glm::vec3 lightPos = pointLightPositions[i];
                float aspect = (float) SHADOW_WIDTH / (float) SHADOW_HEIGHT;
                float near = 0.1f;
                float far = 25.0f;
                glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);

                std::vector<glm::mat4> shadowTransforms;
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * 
                    glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));
                
                pointShadowShaders.use();
                for (int j = 0; j < 6; ++j) {
                    pointShadowShaders.setMat4(shadowMatrixNames[j], shadowTransforms[j]);
                }

                pointShadowShaders.setFloat("far", far);
                pointShadowShaders.setVec3("lightPos", lightPos);

                glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
                glBindFramebuffer(GL_FRAMEBUFFER, shadowCubemapFBO[i]);
                glClear(GL_DEPTH_BUFFER_BIT);
                glActiveTexture(GL_TEXTURE0);

                // Render scene with loaded models
                model = glm::mat4(1.0f);
                model = glm::translate(model, scenePos);
                model = glm::scale(model, sceneScale);
                // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                pointShadowShaders.setMat4("model", model);
                scene.Draw(pointShadowShaders);

                model =  glm::mat4(1.0f);
                model = glm::translate(model, lampPos);
                model = glm::scale(model, lampScale);
                model = glm::rotate(model, glm::radians(ROTATION_SPEED) * (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
                pointShadowShaders.setMat4("model", model);
                lamp.Draw(pointShadowShaders);

                phongShaders.use();
                phongShaders.setInt(shadowCubemapNames[i], 10 + i);
                glActiveTexture(GL_TEXTURE10 + i);
                glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap[i]);
            }

            glCullFace(GL_BACK);
        }
        
        ///////////////////////////////////
        // 2. PASS: RENDER SCENE TO QUAD //
        ///////////////////////////////////
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, quadFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cam.pos, cam.pos + cam.front, cam.worldUp);
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(cam.fov), (float) WINDOW_WIDTH / (float) WINDOW_HEIGHT, NEAR_PLANE, FAR_PLANE);

        if (shaderType == PHONG) {
            phongShaders.use();
            phongShaders.setMat4("projection", projection);
            phongShaders.setMat4("view", view);
            phongShaders.setVec3("viewPos", cam.pos);
        }
        else if (shaderType == CONSTANT) {
            constantShaders.use();
            constantShaders.setMat4("projection", projection);
            constantShaders.setMat4("view", view);
        }
        else if (shaderType == DEPTH) {
            depthShaders.use();
            depthShaders.setFloat("near", NEAR_PLANE);
            depthShaders.setFloat("far", FAR_PLANE);
            depthShaders.setMat4("projection", projection);
            depthShaders.setMat4("view", view);
        }

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, scenePos);
        model = glm::scale(model, sceneScale);
        // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

        if (shaderType == PHONG) {
            phongShaders.setMat4("model", model);
            phongShaders.setMat3("normalMatrix", normalMatrix);
            phongShaders.setFloat("transparency", 1.0f);
            scene.Draw(phongShaders);
        }
        else if (shaderType == CONSTANT) {
            constantShaders.setMat4("model", model);
            constantShaders.setMat3("normalMatrix", normalMatrix);
            constantShaders.setFloat("transparency", 1.0f);
            scene.Draw(constantShaders);
        }
        else if (shaderType == DEPTH) {
            depthShaders.setMat4("model", model);
            depthShaders.setMat3("normalMatrix", normalMatrix);
            scene.Draw(depthShaders);
        }

        model = glm::mat4(1.0f);
        model = glm::translate(model, lampPos);
        model = glm::scale(model, lampScale);
        model = glm::rotate(model, glm::radians(ROTATION_SPEED) * (float) glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        
        normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
        if (shaderType == PHONG) {
            phongShaders.setMat4("model", model);
            phongShaders.setMat3("normalMatrix", normalMatrix);
            phongShaders.setFloat("transparency", 1.0f);
            lamp.Draw(phongShaders);
        }
        else if (shaderType == CONSTANT) {
            constantShaders.setMat4("model", model);
            constantShaders.setMat3("normalMatrix", normalMatrix);
            constantShaders.setFloat("transparency", 1.0f);
            lamp.Draw(constantShaders);
        }
        else if (shaderType == DEPTH) {
            depthShaders.setMat4("model", model);
            depthShaders.setMat3("normalMatrix", normalMatrix);
            lamp.Draw(depthShaders);
        }

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

        lightSourceShaders.setVec3("lightColor", pointLightColor3);
        model = glm::mat4(1.0f);
        model = glm::translate(model, pointLightPositions[2]);
        model = glm::scale(model, glm::vec3(0.2f));
        lightSourceShaders.setMat4("model", model);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // lightSourceShaders.setVec3("lightColor", pointLightColor4);
        // model = glm::mat4(1.0f);
        // model = glm::translate(model, pointLightPositions[3]);
        // model = glm::scale(model, glm::vec3(0.2f));
        // lightSourceShaders.setMat4("model", model);
        // glBindVertexArray(lightVAO);
        // glDrawArrays(GL_TRIANGLES, 0, 36);


        // Directional light
        // lightSourceShaders.setVec3("lightColor", dirLightColor);
        // model = glm::mat4(1.0f);
        // model = glm::translate(model, dirLightPos); 
        // // model = glm::scale(model, glm::vec3(1.0f));
        // lightSourceShaders.setMat4("model", model);
        // glBindVertexArray(lightVAO);
        // glDrawArrays(GL_TRIANGLES, 0, 36);

        // // Debug line
        // glm::vec3 lineStart = dirLightPos;
        // glm::vec3 lineEnd = dirLightPos + glm::normalize(lightDirection) * 5.0f; 

        // float lineVertices[] = {
        //     lineStart.x, lineStart.y, lineStart.z,
        //     lineEnd.x, lineEnd.y, lineEnd.z
        // };

        // glBindVertexArray(lineVAO);
        // glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);

        // lightSourceShaders.use();
        // lightSourceShaders.setMat4("projection", projection);
        // lightSourceShaders.setMat4("view", view);
        // lightSourceShaders.setMat4("model", glm::mat4(1.0f)); 
        // lightSourceShaders.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.0f)); // Yellow line

        // glDrawArrays(GL_LINES, 0, 2);
        // glBindVertexArray(0);

        ////////////////////////////////////
        // 3. PASS: RENDER SKYBOX TO QUAD //
        ////////////////////////////////////
        if (skyboxToggle)
        {
            glDepthFunc(GL_LEQUAL);
            glDisable(GL_CULL_FACE);
            skyboxShaders.use();
            view = glm::mat4(glm::mat3(cam.getViewMatrix()));
            skyboxShaders.setMat4("view", view);
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            skyboxShaders.setMat4("rotation", rotation);
            skyboxShaders.setMat4("projection", projection);
            skyboxShaders.setInt("skybox", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, skybox);
            glBindVertexArray(lightVAO); // repurpose light cube
            glDrawArrays(GL_TRIANGLES, 0, 36);
    
            // glBindVertexArray(0); // unnecessary?
            glEnable(GL_CULL_FACE);
            glDepthFunc(GL_LESS);
    
        }

        ////////////////////////////////////
        // 4. PASS: RENDER QUAD TO SCREEN //
        ////////////////////////////////////

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        quadShaders.use();
        glBindVertexArray(quadVAO);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, quadTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);



        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (fpsToggle)
    {
        std::cout << std::endl;
    }

    glfwTerminate();
    return 0;
}


GLuint loadCubemap(vector<std::string> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (size_t i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

GLuint loadEquirectangularMap(const char* path)
{
    // Equirectangular maps usually require flipping the Y-axis during load
    stbi_set_flip_vertically_on_load(true); 
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum internalFormat = GL_RGB;
        GLenum dataFormat = GL_RGB;
        
        if (nrChannels == 3) {
            internalFormat = GL_SRGB;
            dataFormat = GL_RGB;
        } else if (nrChannels == 4) {
            internalFormat = GL_SRGB_ALPHA;
            dataFormat = GL_RGBA;
        } else if (nrChannels == 1) {
            internalFormat = GL_RED;
            dataFormat = GL_RED;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load equirectangular map: " << path << std::endl;
        stbi_image_free(data);
    }
    
    stbi_set_flip_vertically_on_load(false);
    return textureID;
}

glm::vec3 srgbToLinear(glm::vec3 srgb) {
    return glm::vec3(pow(srgb.r, 2.2f), pow(srgb.g, 2.2f), pow(srgb.b, 2.2f));
}