#include <iostream>
#include <math.h>
#include <ctime>
#include <vector>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <cctype>
#include <utility>
#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <argparse.hpp>
#include "Camera.hpp"
#include "Shader.hpp"
#include "Model.hpp"
#include "Scene.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xPos, double yPos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window);
void generateCube(GLuint &VAO, GLuint &VBO);
void generateQuad(GLuint &FBO, GLuint &VBO, GLuint &VAO, GLuint &texture, GLuint &rboDepth);
void generateLine(GLuint &VBO, GLuint &VAO, int lineCount = 1);
void generateSkybox(GLuint &VAO, GLuint &VBO, GLuint &equirectangularMap, GLuint &cubemap, GLuint &captureFBO, Shader &er2cubemapShader, Scene &scene);
void generateShadowMap(GLuint &FBO, GLuint &texture);
void generateShadowCubemap(GLuint &shadowCubemapFBO, GLuint &shadowCubemap);
void generateIrradianceMap(GLuint &irradianceMap, GLuint &captureFBO, Shader &irradianceShaders, GLuint &skyboxCubemap, GLuint &cubeVAO);
void generatePrefilterMap(GLuint &prefilterMap, GLuint &captureFBO, Shader &prefilterShader, GLuint &skyboxCubemap, GLuint &cubeVAO);
void generateBRDFLUT(GLuint &brdfLUTTexture, GLuint &captureFBO, Shader &brdfShaders, GLuint &quadVAO);
GLuint loadCubemap(const std::vector<std::string>& faces);
GLuint loadEquirectangularMap(const char* path);
glm::vec3 srgbToLinear(glm::vec3 srgb);
void resetScreenshotDir();
void captureScreenshot(GLFWwindow* window, const std::string& filename);

// Global variables
bool firstMouse = true;
bool skyboxToggle = true;
bool fpsToggle = true;
bool vSyncToggle = true;
bool debugToggle = false;
bool normalToggle = true;
bool lightToggle = true;
bool iblToggle = true;
bool specularIBLOnlyMirror = false;
bool environmentChanged = false;
bool orbitMode = false;
bool hasSplats = false;
bool debugMode = false;

const int DIR_SHADOW_WIDTH = 2048;
const int DIR_SHADOW_HEIGHT = 2048;
const int POINT_SHADOW_WIDTH = 1024;
const int POINT_SHADOW_HEIGHT = 1024;
const int SKYBOX_WIDTH = 2048;
const int SKYBOX_HEIGHT = 2048;
const int IRRADIANCE_WIDTH = 32;
const int IRRADIANCE_HEIGHT = 32;
const int PREFILTER_WIDTH = 256;
const int PREFILTER_HEIGHT = 256;

constexpr int MAX_POINT_LIGHTS = 10;

int WINDOW_WIDTH = 2560;
int WINDOW_HEIGHT = 1440;
int frameCount = 0;

const float NEAR_PLANE = 0.5f;
const float FAR_PLANE = 100.0f;
const float ROTATION_SPEED = 275.0f;
const float globalAmbient = 0.08f;
const float globalLightScale = 1.0f;
const float orthoScale = 2.0f;
const float POINT_LIGHT_LINE_SCALE = 5.0f;
const float DEBUG_DURATION = 5.0f;
const float DEBUG_INTERVAL = 1.0f;
const int DEBUG_MAX_SCREENSHOTS = 5;

float debugTimer = 0.0f;
int debugScreenshotsTaken = 0;

float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
float exposure = 1.0f;
float fpsTimer = 0.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 initialCamPos = glm::vec3(-5.0f, 10.0f, 30.0f);

Camera cam = Camera(initialCamPos);

enum ShaderType {
    PBR,
    PHONG,
    CONSTANT,
    DEPTH,
    SPLAT_PBR,
    SPLAT
};
const int SHADER_TYPE_COUNT = 6;
int shaderType = PBR;

enum Environment {
    URBAN,
    SEA,
    INDUSTRIAL,
    GARDEN,
    DESERT,
    SPACE
};
int environment = URBAN;

std::string getSkyboxPath(Environment env) {
    switch (env) {
        case URBAN:
            return "assets/skybox/urban.hdr";
        case INDUSTRIAL:
            return "assets/skybox/industrial.hdr";
        case SEA:
            return "assets/skybox/lakeside.hdr";
        case GARDEN:
            return "assets/skybox/garden.hdr";
        case DESERT:
            return "assets/skybox/desert.hdr";
        case SPACE:
            return "assets/skybox/space_8x.png";
        default:
            return "";
    }
}

int main(int argc, char* argv[]) {

    // Initialize argparse
    argparse::ArgumentParser program("LearnOpenGL");
    program.add_argument("input_scene").help("Path to the input scene.json");
    program.add_argument("--orbit")
        .help("Enable orbit camera mode (constantly circles the scene origin)")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--debug")
        .help("Agent diagnostic mode: empties the screenshots folder, renders in orbit mode for 5 seconds and saves one screenshot per second")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--resolution", "-r")
        .help("Window size as WIDTHxHEIGHT, e.g. 1920x1080")
        .default_value("1920x1080");
    program.add_argument("--culling")
        .help("Cull splats with opacity below the given alpha threshold in 1/255 units (1-255); no culling if omitted")
        .default_value("");
    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << "Argparse error: " << err.what() << '\n';
        std::cerr << program;
        return 1;
    }
    orbitMode = program.get<bool>("--orbit");
    debugMode = program.get<bool>("--debug");
    if (debugMode) {orbitMode = true;}
    std::string sceneName = program.get<std::string>("input_scene");
    std::string scene_path = "scenes/" + sceneName + ".json";

    std::string resArg = program.get<std::string>("--resolution");
    for (char& c : resArg) {
        c = std::tolower(static_cast<unsigned char>(c));
    }
    static const std::unordered_map<std::string, std::pair<int, int>> resPresets = {
        {"hd", {1280, 720}},
        {"fhd", {1920, 1080}},
        {"qhd", {2560, 1440}},
        {"4k", {3840, 2160}},
    };
    size_t xPos = resArg.find('x');
    try {
        auto it = resPresets.find(resArg);
        int resW, resH;
        if (xPos == std::string::npos || xPos == 0 || xPos == resArg.size() - 1) {
            if (it == resPresets.end())
                throw std::invalid_argument(resArg);
            std::tie(resW, resH) = it->second;
        } else {
            resW = std::stoi(resArg.substr(0, xPos));
            resH = std::stoi(resArg.substr(xPos + 1));
        }
        if (resW <= 0 || resH <= 0 || resW > 16384 || resH > 16384)
            throw std::out_of_range(resArg);
        WINDOW_WIDTH = resW;
        WINDOW_HEIGHT = resH;
        lastX = WINDOW_WIDTH / 2.0f;
        lastY = WINDOW_HEIGHT / 2.0f;
    } catch (const std::exception&) {
        std::cerr << "Argparse error: argument --resolution/-r: invalid value '" << resArg << "' (expected WIDTHxHEIGHT, e.g. 1920x1080, or a preset: hd, fhd, qhd, 4k)\n";
        std::cerr << program;
        return 1;
    }

    // Optional splat opacity culling: threshold in 1/255 alpha units (1-255);
    // an empty default means no culling
    int cullingValue = 0;
    {
        std::string cullArg = program.get<std::string>("--culling");
        if (!cullArg.empty()) {
            try {
                size_t parseEnd = 0;
                cullingValue = std::stoi(cullArg, &parseEnd);
                if (parseEnd != cullArg.size()) throw std::invalid_argument(cullArg);
                if (cullingValue < 1 || cullingValue > 255) throw std::out_of_range(cullArg);
            } catch (const std::exception&) {
                std::cerr << "Argparse error: argument --culling: invalid value '" << cullArg << "' (expected an integer between 1 and 255)\n";
                std::cerr << program;
                return 1;
            }
        }
    }

    // Initialize GLFW
    if (debugMode) {
        resetScreenshotDir();
        std::cout << "Debug mode: screenshots folder emptied, saving one screenshot per second for " << DEBUG_DURATION << " seconds in orbit mode\n";
    }
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
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

    // Center window
    int monitorX, monitorY, monitorWidth, monitorHeight;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &monitorX, &monitorY, &monitorWidth, &monitorHeight);

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    glfwSetWindowPos(
        window,
        monitorX + (monitorWidth - windowWidth) / 2,
        monitorY + (monitorHeight - windowHeight) / 2
    );

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
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    {
        // Define shaders
        std::cout << "Compiling shaders...\n";
        Shader phongShaders("shaders/vertex/vs.glsl", "shaders/fragment/fs_phong.glsl");
        Shader pbrShaders("shaders/vertex/vs.glsl", "shaders/fragment/fs_pbr.glsl");
        Shader constantShaders("shaders/vertex/vs.glsl", "shaders/fragment/fs_constant.glsl");
        Shader lightSourceShaders("shaders/vertex/vs_lightSource.glsl", "shaders/fragment/fs_lightSource.glsl");
        Shader er2cubemapShaders("shaders/vertex/vs_skybox.glsl", "shaders/fragment/fs_er2cubemap.glsl");
        Shader skyboxShaders("shaders/vertex/vs_skybox.glsl", "shaders/fragment/fs_skybox.glsl");
        Shader dirShadowShaders("shaders/vertex/vs_dirShadows.glsl", "shaders/fragment/fs_dirShadows.glsl");
        Shader pointShadowShaders("shaders/vertex/vs_pointShadows.glsl", "shaders/fragment/fs_pointShadows.glsl", "shaders/geometry/gs_pointShadows.glsl");
        Shader debugShaders("shaders/vertex/vs_debug.glsl", "shaders/fragment/fs_debug.glsl");
        Shader depthShaders("shaders/vertex/vs.glsl", "shaders/fragment/fs_depth.glsl");
        Shader quadShaders("shaders/vertex/vs_quad.glsl", "shaders/fragment/fs_quad.glsl");
        Shader irradianceShaders("shaders/vertex/vs_skybox.glsl", "shaders/fragment/fs_irradiance.glsl");
        Shader prefilterShaders("shaders/vertex/vs_skybox.glsl", "shaders/fragment/fs_prefilter.glsl");
        Shader brdfShaders("shaders/vertex/vs_quad.glsl", "shaders/fragment/fs_brdf.glsl");
        Shader splatShaders("shaders/vertex/vs_splat.glsl", "shaders/fragment/fs_splat.glsl");
        Shader splatPbrShaders("shaders/vertex/vs_splat.glsl", "shaders/fragment/fs_splat_pbr.glsl");

        quadShaders.use();
        quadShaders.setInt("screenTexture", 0);

        // stbi_set_flip_vertically_on_load(true);

        // Load models and scene parameters from JSON
        Scene scene;
        scene.splatOpacityCullThreshold = cullingValue / 255.0f; // 0 = no culling
        if (!scene.loadFromJSON(scene_path)) {
            std::cerr << "Failed to load scene from JSON." << std::endl;
            return -1;
        }

        if (scene.pointLights.size() > static_cast<size_t>(MAX_POINT_LIGHTS)) {
            std::cerr << "Scene has " << scene.pointLights.size() << " point lights; only "
                      << MAX_POINT_LIGHTS << " are supported." << std::endl;
            return -1;
        }

        const int NUM_POINT_LIGHTS = static_cast<int>(scene.pointLights.size());

        hasSplats = !scene.splatEntities.empty();
        if (hasSplats) {shaderType = SPLAT_PBR;}

        // Light cube vertices
        GLuint cubeVBO, cubeVAO;
        generateCube(cubeVBO, cubeVAO);

        // Skybox
        GLuint skyboxCubemap = 0, captureFBO;
        glGenFramebuffers(1, &captureFBO);
        if (!scene.skyboxPath.empty()) {
            if (scene.skyboxPath.size() > 1) {
                skyboxCubemap = loadCubemap(scene.skyboxPath);
            } 
            else {
                GLuint skyboxEquirectangular = loadEquirectangularMap(scene.skyboxPath[0].c_str());
                generateSkybox(cubeVBO, cubeVAO, skyboxEquirectangular, skyboxCubemap, captureFBO, er2cubemapShaders, scene);
            }
        }

        // Generate irradiance map for IBL
        GLuint irradianceMap;
        generateIrradianceMap(irradianceMap, captureFBO, irradianceShaders, skyboxCubemap, cubeVAO);

        // Pre-allocate uniform strings
        std::string shadowMatrixNames[6];
        for (int j = 0; j < 6; ++j) {
            shadowMatrixNames[j] = "shadowMatrices[" + std::to_string(j) + "]";
        }

        std::vector<std::string> shadowCubemapNames(NUM_POINT_LIGHTS);
        for (int i = 0; i < NUM_POINT_LIGHTS; ++i) {
            shadowCubemapNames[i] = "shadowCubemaps[" + std::to_string(i) + "]";
        }

        std::string skyboxMatrixNames[6];
        for (int i = 0; i < 6; ++i) {
            skyboxMatrixNames[i] = "skyboxTransforms[" + std::to_string(i) + "]";
        }

        // Debug line for directional light + point light rays (14 per light: 6 faces, 8 corners)
        GLuint lineVBO, lineVAO;
        generateLine(lineVBO, lineVAO, NUM_POINT_LIGHTS * 14 + 1);

        // Final render target: screen-filling HDR quad
        GLuint quadFBO, quadVBO, quadVAO, quadTexture, quadRboDepth;
        generateQuad(quadFBO, quadVBO, quadVAO, quadTexture, quadRboDepth);

        // --- 1. PREFILTER MAP GENERATION ---
        GLuint prefilterMap;
        GLuint iblFBO;
        glGenFramebuffers(1, &iblFBO);
        generatePrefilterMap(prefilterMap, iblFBO, prefilterShaders, skyboxCubemap, cubeVAO);

        // --- 2. BRDF LUT GENERATION ---
        GLuint brdfLUTTexture;
        generateBRDFLUT(brdfLUTTexture, iblFBO, brdfShaders, quadVAO);

        // Directional light shadow map FBO and texture
        GLuint shadowMapFBO, shadowMap;
        generateShadowMap(shadowMapFBO, shadowMap);

        // Point light shadow cubemap FBOs and textures
        std::vector<GLuint> shadowCubemapFBO(NUM_POINT_LIGHTS);
        std::vector<GLuint> shadowCubemap(NUM_POINT_LIGHTS);

        for (int i = 0; i < NUM_POINT_LIGHTS; ++i)
        {
            generateShadowCubemap(shadowCubemapFBO[i], shadowCubemap[i]);
        }

        // Configure shaders
        phongShaders.use();
        phongShaders.setInt("numPointLights", NUM_POINT_LIGHTS);
        phongShaders.setFloat("far_plane", 25.0f);
        phongShaders.setFloat("globalAmbient", scene.globalAmbient);

        for (int i = 0; i < 10; i++) { 
            phongShaders.setInt("shadowCubemaps[" + std::to_string(i) + "]", 10 + i);
        }

        if (scene.dirLight.intensity > 0.0f) {
            phongShaders.setVec3("dirLight.color", srgbToLinear(scene.dirLight.color) * scene.dirLight.intensity);
            phongShaders.setVec3("dirLight.position", scene.dirLight.position);
        }

        for (size_t i = 0; i < scene.pointLights.size(); i++) {
            phongShaders.setVec3("pointLights[" + std::to_string(i) + "].color", srgbToLinear(scene.pointLights[i].color) * scene.pointLights[i].intensity);
            phongShaders.setVec3("pointLights[" + std::to_string(i) + "].position", scene.pointLights[i].position);
        }

        pbrShaders.use();
        pbrShaders.setInt("numPointLights", NUM_POINT_LIGHTS); 
        pbrShaders.setFloat("far_plane", 25.0f);
        pbrShaders.setFloat("globalAmbient", globalAmbient);
        specularIBLOnlyMirror = scene.specularIBLOnlyMirror;

        for (int i = 0; i < 10; i++) { 
            pbrShaders.setInt("shadowCubemaps[" + std::to_string(i) + "]", 10 + i);
        }

        if (scene.dirLight.intensity > 0.0f) {
            pbrShaders.setVec3("dirLight.color", srgbToLinear(scene.dirLight.color) * scene.dirLight.intensity);
            pbrShaders.setVec3("dirLight.position", scene.dirLight.position);
        }

        for (size_t i = 0; i < scene.pointLights.size(); i++) {
            pbrShaders.setVec3("pointLights[" + std::to_string(i) + "].color", srgbToLinear(scene.pointLights[i].color) * scene.pointLights[i].intensity);
            pbrShaders.setVec3("pointLights[" + std::to_string(i) + "].position", scene.pointLights[i].position);
        }

        pbrShaders.setInt("irradianceMap", 20);
        glActiveTexture(GL_TEXTURE20);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

        pbrShaders.setInt("prefilterMap", 21);
        glActiveTexture(GL_TEXTURE21);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);

        pbrShaders.setInt("brdfLUT", 22);
        glActiveTexture(GL_TEXTURE22);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

        splatPbrShaders.use();
        splatPbrShaders.setInt("numPointLights", NUM_POINT_LIGHTS);
        splatPbrShaders.setFloat("far_plane", 25.0f);
        splatPbrShaders.setFloat("globalAmbient", globalAmbient);

        for (int i = 0; i < 10; i++) {
            splatPbrShaders.setInt("shadowCubemaps[" + std::to_string(i) + "]", 10 + i);
        }

        if (scene.dirLight.intensity > 0.0f) {
            splatPbrShaders.setVec3("dirLight.color", srgbToLinear(scene.dirLight.color) * scene.dirLight.intensity);
            splatPbrShaders.setVec3("dirLight.position", scene.dirLight.position);
        }

        for (size_t i = 0; i < scene.pointLights.size(); i++) {
            splatPbrShaders.setVec3("pointLights[" + std::to_string(i) + "].color", srgbToLinear(scene.pointLights[i].color) * scene.pointLights[i].intensity);
            splatPbrShaders.setVec3("pointLights[" + std::to_string(i) + "].position", scene.pointLights[i].position);
        }

        splatPbrShaders.setInt("irradianceMap", 20);
        glActiveTexture(GL_TEXTURE20);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

        splatPbrShaders.setInt("prefilterMap", 21);
        glActiveTexture(GL_TEXTURE21);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);

        splatPbrShaders.setInt("brdfLUT", 22);
        glActiveTexture(GL_TEXTURE22);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

        skyboxShaders.use();
        skyboxShaders.setInt("skybox", 0);

        //////////////////////
        // MAIN RENDER LOOP //
        //////////////////////
        glfwShowWindow(window);
        while (!glfwWindowShouldClose(window))
        {
            // Input
            processInput(window);
            quadShaders.use();
            if (shaderType == PHONG) {quadShaders.setFloat("exposure", exposure - 0.5f);}
            else {quadShaders.setFloat("exposure", exposure);}
            phongShaders.use();
            phongShaders.setBool("normalToggle", normalToggle);
            phongShaders.setBool("lightToggle", lightToggle);
            phongShaders.setInt("numPointLights", lightToggle ? NUM_POINT_LIGHTS : 0);
            pbrShaders.use();
            pbrShaders.setBool("normalToggle", normalToggle);
            pbrShaders.setBool("lightToggle", lightToggle);
            pbrShaders.setInt("numPointLights", lightToggle ? NUM_POINT_LIGHTS : 0);
            pbrShaders.setBool("specularIBLOnlyMirror", specularIBLOnlyMirror);
            splatPbrShaders.use();
            splatPbrShaders.setBool("lightToggle", lightToggle);
            splatPbrShaders.setInt("numPointLights", lightToggle ? NUM_POINT_LIGHTS : 0);
            splatPbrShaders.setBool("specularIBLOnlyMirror", specularIBLOnlyMirror);
            if (environmentChanged) {
                std::string newSkyboxPath = getSkyboxPath(static_cast<Environment>(environment));
                if (!newSkyboxPath.empty()) {
                    // 1. Delete previous environment textures from VRAM
                    if (skyboxCubemap) glDeleteTextures(1, &skyboxCubemap);
                    if (irradianceMap) glDeleteTextures(1, &irradianceMap);
                    if (prefilterMap)  glDeleteTextures(1, &prefilterMap);

                    // 2. Load intermediate equirectangular HDRI
                    GLuint newSkyboxEquirectangular = loadEquirectangularMap(newSkyboxPath.c_str());

                    // 3. Generate cubemaps (reusing existing captureFBO)
                    generateSkybox(cubeVBO, cubeVAO, newSkyboxEquirectangular, skyboxCubemap, captureFBO, er2cubemapShaders, scene);
                    generateIrradianceMap(irradianceMap, captureFBO, irradianceShaders, skyboxCubemap, cubeVAO);
                    generatePrefilterMap(prefilterMap, captureFBO, prefilterShaders, skyboxCubemap, cubeVAO);

                    // 4. Free the intermediate equirectangular texture immediately
                    glDeleteTextures(1, &newSkyboxEquirectangular);

                    // 5. Rebind updated textures to their respective texture units
                    pbrShaders.use();
                    pbrShaders.setInt("irradianceMap", 20);
                    glActiveTexture(GL_TEXTURE20);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

                    pbrShaders.setInt("prefilterMap", 21);
                    glActiveTexture(GL_TEXTURE21);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);

                    splatPbrShaders.use();
                    splatPbrShaders.setInt("irradianceMap", 20);
                    glActiveTexture(GL_TEXTURE20);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

                    splatPbrShaders.setInt("prefilterMap", 21);
                    glActiveTexture(GL_TEXTURE21);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
                }
                environmentChanged = false;
            }
            pbrShaders.use();
            pbrShaders.setBool("iblToggle", iblToggle);
            splatPbrShaders.use();
            splatPbrShaders.setBool("iblToggle", iblToggle);

            // Clear background
            if (shaderType == DEPTH) {
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            } else {
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            }
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Define delta time
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            scene.update(deltaTime);

            // Orbit camera mode
            if (orbitMode) {
                cam.processOrbit(deltaTime, scene.orbit.radius, scene.orbit.height, scene.orbit.speed);
            }

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
            // (hoisted so the splat pass can reuse it for receiving shadows)
            glm::mat4 dirLightSpaceMatrix = glm::mat4(1.0f);
            if (lightToggle && (shaderType == PHONG or shaderType == PBR or shaderType == SPLAT_PBR)) {

                // 1.1: Render directional light shadow map
                if (scene.dirLight.intensity > 0.0f) {
                    glm::mat4 lightProjection = glm::ortho(orthoScale * -10.0f, orthoScale * 10.0f, orthoScale * -10.0f, orthoScale * 10.0f, NEAR_PLANE, FAR_PLANE);
                    glm::mat4 lightView = glm::lookAt(scene.dirLight.position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
                    dirLightSpaceMatrix = lightSpaceMatrix;

                    glCullFace(GL_FRONT);

                    dirShadowShaders.use();
                    dirShadowShaders.setMat4("dirLightSpaceMatrix", lightSpaceMatrix);

                    glViewport(0, 0, DIR_SHADOW_WIDTH, DIR_SHADOW_HEIGHT);
                    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
                    glClear(GL_DEPTH_BUFFER_BIT);
                    glActiveTexture(GL_TEXTURE0);

                    // Render scene with loaded models
                    for (const auto& entity : scene.entities) {
                        glm::mat4 modelMatrix = entity.transform.getMatrix();
                        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
                        dirShadowShaders.setMat4("model", modelMatrix);
                        entity.model->drawOpaque(dirShadowShaders);
                    }

                    glBindFramebuffer(GL_FRAMEBUFFER, 0);

                    if (shaderType == PHONG) {
                        phongShaders.use();
                        phongShaders.setInt("shadowMap", 9); // 1 -> 9, Gemini suggestion
                        phongShaders.setMat4("dirLightSpaceMatrix", lightSpaceMatrix);
                        glActiveTexture(GL_TEXTURE9);
                        glBindTexture(GL_TEXTURE_2D, shadowMap);
                    } else if (shaderType == PBR) {
                        pbrShaders.use();
                        pbrShaders.setInt("shadowMap", 9); // 1 -> 9, Gemini suggestion
                        pbrShaders.setMat4("dirLightSpaceMatrix", lightSpaceMatrix);
                        glActiveTexture(GL_TEXTURE9);
                        glBindTexture(GL_TEXTURE_2D, shadowMap);
                    } else if (shaderType == SPLAT_PBR) {
                        splatPbrShaders.use();
                        splatPbrShaders.setInt("shadowMap", 9);
                        splatPbrShaders.setMat4("dirLightSpaceMatrix", lightSpaceMatrix);
                        glActiveTexture(GL_TEXTURE9);
                        glBindTexture(GL_TEXTURE_2D, shadowMap);
                    }
                }

                // 1.2: Render point light shadow cubemaps
                if (lightToggle)
                {
                    for (int i = 0; i < NUM_POINT_LIGHTS; i++) {

                        glm::vec3 lightPos = scene.pointLights[i].position;
                        float aspect = (float) POINT_SHADOW_WIDTH / (float) POINT_SHADOW_HEIGHT;
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

                        glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);
                        glBindFramebuffer(GL_FRAMEBUFFER, shadowCubemapFBO[i]);
                        glClear(GL_DEPTH_BUFFER_BIT);
                        glActiveTexture(GL_TEXTURE0);

                        // Render scene with loaded models
                        for (const auto& entity : scene.entities) {
                            glm::mat4 modelMatrix = entity.transform.getMatrix();
                            pointShadowShaders.setMat4("model", modelMatrix);
                            entity.model->drawOpaque(pointShadowShaders);
                        }

                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    } 

                    for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
                        if (shaderType == PHONG) {
                            phongShaders.use();
                            phongShaders.setInt(shadowCubemapNames[i], 10 + i);
                            glActiveTexture(GL_TEXTURE10 + i);
                            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap[i]);
                        } else if (shaderType == PBR) {
                            pbrShaders.use();
                            pbrShaders.setInt(shadowCubemapNames[i], 10 + i);
                            glActiveTexture(GL_TEXTURE10 + i);
                            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap[i]);
                        } else if (shaderType == SPLAT_PBR) {
                            splatPbrShaders.use();
                            splatPbrShaders.setInt(shadowCubemapNames[i], 10 + i);
                            glActiveTexture(GL_TEXTURE10 + i);
                            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap[i]);
                        }
                    }
                }
                glCullFace(GL_BACK);
            }

            ////////////////////////////////////////////
            // 2. PASS: RENDER OPAQUE OBJECTS TO QUAD //
            ////////////////////////////////////////////
            glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, quadFBO);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = glm::lookAt(cam.pos, cam.pos + cam.front, cam.worldUp);
            glm::mat4 projection = glm::mat4(1.0f);
            projection = glm::perspective(glm::radians(cam.fov), (float) WINDOW_WIDTH / (float) WINDOW_HEIGHT, NEAR_PLANE, FAR_PLANE);

            for (const auto& entity : scene.entities) {
                glm::mat4 model = entity.transform.getMatrix();
                glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

                if (shaderType == PHONG) {
                    phongShaders.use();
                    phongShaders.setMat4("projection", projection);
                    phongShaders.setMat4("view", view);
                    phongShaders.setVec3("viewPos", cam.pos);
                    phongShaders.setMat4("model", model);
                    phongShaders.setMat3("normalMatrix", normalMatrix);
                    phongShaders.setFloat("transparency", 1.0f);
                    entity.model->drawOpaque(phongShaders);
                }
                else if (shaderType == PBR) {
                    pbrShaders.use();
                    pbrShaders.setMat4("projection", projection);
                    pbrShaders.setMat4("view", view);
                    pbrShaders.setVec3("viewPos", cam.pos);
                    pbrShaders.setMat4("model", model);
                    pbrShaders.setMat3("normalMatrix", normalMatrix);
                    pbrShaders.setFloat("transparency", 1.0f);
                    entity.model->drawOpaque(pbrShaders);
                }
                else if (shaderType == CONSTANT) {
                    constantShaders.use();
                    constantShaders.setMat4("projection", projection);
                    constantShaders.setMat4("view", view);
                    constantShaders.setMat4("model", model);
                    entity.model->draw(constantShaders);
                }
                else if (shaderType == DEPTH) {
                    depthShaders.use();
                    depthShaders.setFloat("near", NEAR_PLANE);
                    depthShaders.setFloat("far", FAR_PLANE);
                    depthShaders.setMat4("projection", projection);
                    depthShaders.setMat4("view", view);
                    depthShaders.setMat4("model", model);
                    depthShaders.setMat3("normalMatrix", normalMatrix);
                    entity.model->draw(depthShaders);
                }
            }

            ////////////////////////////////////////////
            // RENDER LIGHT SOURCES FOR VISUALIZATION //
            ////////////////////////////////////////////
            glm::vec3 pointLightCubeScale = glm::vec3(0.02f);

            if (debugToggle) {
                lightSourceShaders.use();
                lightSourceShaders.setMat4("projection", projection);
                lightSourceShaders.setMat4("view", view);

                for (size_t i = 0; i < scene.pointLights.size(); i++) {
                    lightSourceShaders.setVec3("lightColor", scene.pointLights[i].color * scene.pointLights[i].intensity);
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, scene.pointLights[i].position);
                    model = glm::scale(model, pointLightCubeScale);
                    lightSourceShaders.setMat4("model", model);
                    glBindVertexArray(cubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // Directional light
                if (scene.dirLight.intensity > 0.0f) {
                    lightSourceShaders.setVec3("lightColor", scene.dirLight.color * scene.dirLight.intensity);
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, scene.dirLight.position); 
                    // model = glm::scale(model, glm::vec3(1.0f));
                    lightSourceShaders.setMat4("model", model);
                    glBindVertexArray(cubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // // Debug line
                glm::vec3 lineStart = scene.dirLight.position;
                glm::vec3 lineEnd = glm::vec3(0.0f, 0.0f, 0.0f); // Pointing towards the origin

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
                lightSourceShaders.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.0f));

                glDrawArrays(GL_LINES, 0, 2);

                // Point light rays: 14 per light (6 face normals, 8 corners)
                if (NUM_POINT_LIGHTS > 0) {
                    float lineLength = POINT_LIGHT_LINE_SCALE * pointLightCubeScale.x;
                    std::vector<float> rayVertices;
                    rayVertices.reserve(scene.pointLights.size() * 14 * 6);

                    for (const auto& light : scene.pointLights) {
                        for (int j = 0; j < 14; j++) {
                            glm::vec3 dir;
                            if (j < 6) {
                                dir = glm::vec3(0.0f);
                                dir[j % 3] = (j < 3) ? 1.0f : -1.0f;
                            } else {
                                int k = j - 6;
                                dir = glm::normalize(glm::vec3(
                                    (k & 1) ? 1.0f : -1.0f,
                                    (k & 2) ? 1.0f : -1.0f,
                                    (k & 4) ? 1.0f : -1.0f));
                            }
                            glm::vec3 end = light.position + dir * lineLength;
                            rayVertices.push_back(light.position.x);
                            rayVertices.push_back(light.position.y);
                            rayVertices.push_back(light.position.z);
                            rayVertices.push_back(end.x);
                            rayVertices.push_back(end.y);
                            rayVertices.push_back(end.z);
                        }
                    }

                    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, rayVertices.size() * sizeof(float), rayVertices.data());
                    glDrawArrays(GL_LINES, 0, NUM_POINT_LIGHTS * 28);
                }

                glBindVertexArray(0);
            }

            ////////////////////////////////////
            // 3. PASS: RENDER SKYBOX TO QUAD //
            ////////////////////////////////////
            if (!scene.skyboxPath.empty() && skyboxToggle &&
                (shaderType == PHONG or shaderType == PBR or shaderType == SPLAT or shaderType == SPLAT_PBR))
            {
                glDepthFunc(GL_LEQUAL);
                glDisable(GL_CULL_FACE);

                skyboxShaders.use();
                glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
                skyboxShaders.setMat4("view", skyboxView);
                skyboxShaders.setMat4("projection", projection);
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(scene.skyboxRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                rotation = glm::rotate(rotation, glm::radians(scene.skyboxRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                rotation = glm::rotate(rotation, glm::radians(scene.skyboxRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                skyboxShaders.setMat4("rotation", rotation);
                skyboxShaders.setInt("skybox", 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
                glBindVertexArray(cubeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                glEnable(GL_CULL_FACE);
                glDepthFunc(GL_LESS);

            }
            /////////////////////////////////////////
            // 4. PASS: RENDER TRANSPARENT OBJECTS //
            /////////////////////////////////////////
            glDepthMask(GL_FALSE);

            for (const auto& entity : scene.entities) {
                glm::mat4 model = entity.transform.getMatrix();
                glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

                if (shaderType == PHONG) {
                    phongShaders.use();
                    phongShaders.setMat4("projection", projection);
                    phongShaders.setMat4("view", view);
                    phongShaders.setVec3("viewPos", cam.pos);
                    phongShaders.setMat4("model", model);
                    phongShaders.setMat3("normalMatrix", normalMatrix);
                    entity.model->drawTransparent(phongShaders);
                }
                else if (shaderType == PBR) {
                    pbrShaders.use();
                    pbrShaders.setMat4("projection", projection);
                    pbrShaders.setMat4("view", view);
                    pbrShaders.setVec3("viewPos", cam.pos);
                    pbrShaders.setMat4("model", model);
                    pbrShaders.setMat3("normalMatrix", normalMatrix);
                    entity.model->drawTransparent(pbrShaders);
                }
            }

            glDepthMask(GL_TRUE);

            ////////////////////////////////////////////
            // 5. PASS: RENDER GAUSSIAN SPLATS TO QUAD //
            ////////////////////////////////////////////
            if ((shaderType == SPLAT or shaderType == SPLAT_PBR) && !scene.splatEntities.empty()) {
                // Blended back-to-front, depth-tested against opaque geometry
                // but never writing depth; face culling off (quad winding is
                // per-splat arbitrary)
                glDepthMask(GL_FALSE);
                glDisable(GL_CULL_FACE);

                Shader& splatShader = (shaderType == SPLAT) ? splatShaders : splatPbrShaders;
                splatShader.use();
                splatShader.setMat4("view", view);
                splatShader.setMat4("projection", projection);
                splatShader.setVec2("viewport", glm::vec2((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT));
                splatShader.setVec3("viewPos", cam.pos);
                if (shaderType == SPLAT_PBR) {
                    splatPbrShaders.setMat4("dirLightSpaceMatrix", dirLightSpaceMatrix);
                }

                // Multiple clouds: draw the farthest first (coarse per-entity
                // ordering on top of each cloud's per-splat sort)
                std::vector<std::pair<float, size_t>> splatEntityOrder(scene.splatEntities.size());
                for (size_t i = 0; i < scene.splatEntities.size(); i++) {
                    const SplatEntity& splatEntity = scene.splatEntities[i];
                    glm::vec3 centroidView = glm::vec3(view * splatEntity.transform.getMatrix() * glm::vec4(splatEntity.cloud->centroid, 1.0f));
                    splatEntityOrder[i] = { centroidView.z, i };
                }
                std::sort(splatEntityOrder.begin(), splatEntityOrder.end(),
                          [](const std::pair<float, size_t>& a, const std::pair<float, size_t>& b) {
                              return a.first < b.first;
                          });

                for (const auto& [centroidDepth, entityIndex] : splatEntityOrder) {
                    const SplatEntity& splatEntity = scene.splatEntities[entityIndex];
                    glm::mat4 model = splatEntity.transform.getMatrix();
                    splatEntity.cloud->updateSort(view, model);
                    splatShader.setMat4("model", model);
                    splatShader.setInt("shDegree", splatEntity.cloud->shDegree);
                    splatEntity.cloud->draw();
                }

                glEnable(GL_CULL_FACE);
                glDepthMask(GL_TRUE);
            }

            ////////////////////////////////////
            // 6. PASS: RENDER QUAD TO SCREEN //
            ////////////////////////////////////

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            quadShaders.use();
            glBindVertexArray(quadVAO);
            glDisable(GL_DEPTH_TEST);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, quadTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);

            // Debug screenshot capture: one per second in orbit mode, then exit
            if (debugMode) {
                debugTimer += deltaTime;
                if (debugTimer >= DEBUG_INTERVAL && debugScreenshotsTaken < DEBUG_MAX_SCREENSHOTS) {
                    debugScreenshotsTaken++;
                    captureScreenshot(window, "screenshots/" + sceneName + "_debug_" + std::to_string(debugScreenshotsTaken) + ".png");
                    std::cout << "Debug mode: saved screenshot " << debugScreenshotsTaken << "/" << DEBUG_MAX_SCREENSHOTS << "\n";
                    debugTimer = 0.0f;
                    if (debugScreenshotsTaken >= DEBUG_MAX_SCREENSHOTS)
                        glfwSetWindowShouldClose(window, true);
                }
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        if (fpsToggle)
        {
            std::cout << std::endl;
        }

        glDeleteFramebuffers(1, &captureFBO);
        glDeleteFramebuffers(1, &iblFBO);
        glDeleteFramebuffers(1, &quadFBO);
        glDeleteFramebuffers(1, &shadowMapFBO);
        for (int i = 0; i < NUM_POINT_LIGHTS; ++i) {
            glDeleteFramebuffers(1, &shadowCubemapFBO[i]);
        }
        glDeleteRenderbuffers(1, &quadRboDepth);
        glDeleteTextures(1, &quadTexture);
        glDeleteTextures(1, &shadowMap);
        for (int i = 0; i < NUM_POINT_LIGHTS; ++i) {
            glDeleteTextures(1, &shadowCubemap[i]);
        }
        glDeleteTextures(1, &brdfLUTTexture);
        if (skyboxCubemap) glDeleteTextures(1, &skyboxCubemap);
        if (irradianceMap) glDeleteTextures(1, &irradianceMap);
        if (prefilterMap) glDeleteTextures(1, &prefilterMap);
        glDeleteVertexArrays(1, &cubeVAO);
        glDeleteBuffers(1, &cubeVBO);
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
        glDeleteVertexArrays(1, &lineVAO);
        glDeleteBuffers(1, &lineVBO);

    }
    glfwTerminate();
    return 0;
}

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

    if (orbitMode) { return; }

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
    if ((key == GLFW_KEY_LEFT or key == GLFW_KEY_RIGHT) && action == GLFW_PRESS)
    {
        int step = (key == GLFW_KEY_RIGHT) ? 1 : -1;
        shaderType += step;
        if (!hasSplats && shaderType > DEPTH) {shaderType = PBR;}
        else if (!hasSplats && shaderType < PBR) {shaderType = DEPTH;}
        else if (hasSplats && shaderType > SPLAT) {shaderType = SPLAT_PBR;}
        else if (hasSplats && shaderType < SPLAT_PBR) {shaderType = SPLAT;}
    }
    if (key == GLFW_KEY_0 && action == GLFW_PRESS)
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
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        lightToggle = !lightToggle;
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
    if (key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        if (normalToggle == false) {normalToggle = true;} else {normalToggle = false;}
    }
    if (key == GLFW_KEY_I && action == GLFW_PRESS)
    {
        if (iblToggle == false) {iblToggle = true;} else {iblToggle = false;}
    }
    if (key == GLFW_KEY_PERIOD && action == GLFW_PRESS)
    {
        if (debugToggle == false) {debugToggle = true;} else {debugToggle = false;}
    }
    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        if (specularIBLOnlyMirror == false) {specularIBLOnlyMirror = true;} else {specularIBLOnlyMirror = false;}
    }
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        environment = 0;
        environmentChanged = true;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        environment = 1;
        environmentChanged = true;
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        environment = 2;
        environmentChanged = true;
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
    {
        environment = 3;
        environmentChanged = true;
    }
    if (key == GLFW_KEY_5 && action == GLFW_PRESS)
    {
        environment = 4;
        environmentChanged = true;
    }
    if (key == GLFW_KEY_6 && action == GLFW_PRESS)
    {
        environment = 5;
        environmentChanged = true;
    }

}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (orbitMode) { return; }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {cam.processKeyboard(Camera::FORWARD, deltaTime);}
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {cam.processKeyboard(Camera::BACKWARD, deltaTime);}
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {cam.processKeyboard(Camera::LEFT, deltaTime);}
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {cam.processKeyboard(Camera::RIGHT, deltaTime);}
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {cam.processKeyboard(Camera::UP, deltaTime);}
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {cam.processKeyboard(Camera::DOWN, deltaTime);}
}

void generateCube(GLuint& cubeVBO, GLuint& cubeVAO)
{
    float cubeVertices[] = {
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
    glGenBuffers(1, &cubeVBO);
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
}

void generateQuad(GLuint& quadFBO, GLuint& quadVBO, GLuint& quadVAO, GLuint& quadTexture, GLuint& quadRboDepth)
{
    float quadVertices[] = {  
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
    };	
    glGenFramebuffers(1, &quadFBO);

    glGenTextures(1, &quadTexture);
    glBindTexture(GL_TEXTURE_2D, quadTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, quadFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, quadTexture, 0);

    glGenRenderbuffers(1, &quadRboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, quadRboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, quadRboDepth);

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
}

void generateLine(GLuint& lineVBO, GLuint& lineVAO, int lineCount)
{
    glGenBuffers(1, &lineVBO);
    glGenVertexArrays(1, &lineVAO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineCount * 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

GLuint loadCubemap(const std::vector<std::string>& faces)
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
            GLenum internalFormat;
            GLenum dataFormat;

            if (nrChannels == 3) {
                internalFormat = GL_RGB16F;
                dataFormat = GL_RGB;
            } else if (nrChannels == 4) {
                internalFormat = GL_RGBA16F;
                dataFormat = GL_RGBA;
            } else if (nrChannels == 1) {
                internalFormat = GL_R16F;
                dataFormat = GL_RED;
            }

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data
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
    float* data = stbi_loadf(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;

        if (nrChannels == 3) {
            internalFormat = GL_RGB32F;
            dataFormat = GL_RGB;
        } else if (nrChannels == 4) {
            internalFormat = GL_RGBA32F;
            dataFormat = GL_RGBA;
        } else if (nrChannels == 1) {
            internalFormat = GL_RED;
            dataFormat = GL_RED;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_FLOAT, data);

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

void generateSkybox(GLuint& cubeVBO, GLuint& cubeVAO, GLuint &equirectangularMap, GLuint& skyboxCubemap, GLuint& captureFBO, Shader& er2cubemapShaders, Scene& scene)
{
    glGenTextures(1, &skyboxCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);

    for (int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F, SKYBOX_WIDTH, SKYBOX_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = 
    {
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +X
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // -X
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // +Y
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // -Y
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +Z
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))  // -Z
    };

    er2cubemapShaders.use();
    er2cubemapShaders.setInt("equirectangularMap", 23);
    er2cubemapShaders.setMat4("projection", captureProjection);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(scene.skyboxRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(scene.skyboxRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(scene.skyboxRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    er2cubemapShaders.setMat4("rotation", rotation);

    glActiveTexture(GL_TEXTURE23);
    glBindTexture(GL_TEXTURE_2D, equirectangularMap);

    glViewport(0, 0, SKYBOX_WIDTH, SKYBOX_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (int i = 0; i < 6; ++i)
    {
        er2cubemapShaders.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, skyboxCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // ADD THIS: Generate mipmaps for the captured skybox
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    // Overwrite the previous GL_LINEAR filter to enable trilinear LOD sampling
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
}

void generateIrradianceMap(GLuint &irradianceMap, GLuint &captureFBO, Shader &irradianceShaders, GLuint &skyboxCubemap, GLuint &cubeVAO) {
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F, IRRADIANCE_WIDTH, IRRADIANCE_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = 
    {
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +X
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // -X
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // +Y
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // -Y
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +Z
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))  // -Z
    };
    irradianceShaders.use();
    irradianceShaders.setInt("environmentMap", 0);
    irradianceShaders.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
    glViewport(0, 0, IRRADIANCE_WIDTH, IRRADIANCE_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i) {
        irradianceShaders.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void generatePrefilterMap(GLuint &prefilterMap, GLuint &iblFBO, Shader &prefilterShaders, GLuint &skyboxCubemap, GLuint &cubeVAO) {
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, PREFILTER_WIDTH, PREFILTER_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned int maxMipLevels = 5;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, maxMipLevels - 1);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindFramebuffer(GL_FRAMEBUFFER, iblFBO);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = 
    {
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +X
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // -X
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // +Y
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // -Y
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +Z
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))  // -Z
    };

    prefilterShaders.use();
    prefilterShaders.setInt("environmentMap", 0);
    prefilterShaders.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        unsigned int mipWidth  = PREFILTER_WIDTH * std::pow(0.5, mip);
        unsigned int mipHeight = PREFILTER_HEIGHT * std::pow(0.5, mip);
        glViewport(0, 0, mipWidth, mipHeight);

        float prefilterRoughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterShaders.setFloat("roughness", prefilterRoughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            prefilterShaders.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
}

void generateBRDFLUT(GLuint &brdfLUTTexture, GLuint &iblFBO, Shader &brdfShaders, GLuint &quadVAO) {

    glGenTextures(1, &brdfLUTTexture);
    glActiveTexture(GL_TEXTURE22);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, iblFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);
    glViewport(0, 0, 512, 512);
    brdfShaders.use();
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void generateShadowMap(GLuint& shadowMapFBO, GLuint& shadowMap)
{
    glGenFramebuffers(1, &shadowMapFBO);

    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, DIR_SHADOW_WIDTH, DIR_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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
}

void generateShadowCubemap(GLuint& shadowCubemapFBO, GLuint& shadowCubemap)
{
    glGenFramebuffers(1, &shadowCubemapFBO);
    glGenTextures(1, &shadowCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap);

    for (int j = 0; j < 6; ++j)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_DEPTH_COMPONENT, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowCubemapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
}

glm::vec3 srgbToLinear(glm::vec3 srgb) {
    return glm::vec3(pow(srgb.r, 2.2f), pow(srgb.g, 2.2f), pow(srgb.b, 2.2f));
}

void resetScreenshotDir() {
    const std::filesystem::path dir("screenshots");
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        std::error_code ec;
        if (entry.is_directory(ec)) {
            std::filesystem::remove_all(entry.path(), ec);
        } else {
            std::filesystem::remove(entry.path(), ec);
        }
        if (ec) {
            std::cerr << "Debug mode: failed to remove '" << entry.path().string() << "': " << ec.message() << '\n';
        }
    }
}

// Writes the default framebuffer (tone-mapped back buffer) as a PNG via stb_image_write, rows flipped vertically
void captureScreenshot(GLFWwindow* window, const std::string& filename) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) {
        std::cerr << "Debug mode: invalid framebuffer size, skipping screenshot\n";
        return;
    }

    std::vector<unsigned char> pixels(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Flip vertically: OpenGL's origin is bottom-left, image formats expect top-left
    std::vector<unsigned char> flipped(pixels.size());
    for (int y = 0; y < height; y++) {
        const unsigned char* src = &pixels[(height - 1 - y) * width * 3];
        std::memcpy(&flipped[y * width * 3], src, width * 3);
    }

    if (!stbi_write_png(filename.c_str(), width, height, 3, flipped.data(), width * 3)) {
        std::cerr << "Debug mode: failed to write '" << filename << "'\n";
    }
}