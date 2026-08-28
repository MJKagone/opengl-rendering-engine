#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.hpp"
#include <json.hpp>

using json = nlohmann::json;

inline glm::vec3 parseVec3(const json& j) {
    return glm::vec3(j[0], j[1], j[2]);
}

struct Transform {
    glm::vec3 position;
    glm::vec3 rotation; // Current rotation in degrees
    glm::vec3 rotationVelocity = glm::vec3(0.0f); // Degrees per second
    glm::vec3 scale;

    glm::mat4 getMatrix() const {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, position);
        m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::scale(m, scale);
        return m;
    }
};

struct Entity {
    std::string name;
    Model* model;
    Transform transform;
};

struct PointLightData {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
};

struct DirLightData {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(0.0f);
    float intensity = 0.0f;
};

class Scene {
public:
    std::vector<Entity> entities;
    std::vector<PointLightData> pointLights;
    DirLightData dirLight;
    
    float globalAmbient = 0.08f;
    float exposure = 1.0f;
    std::vector<std::string> skyboxPath;
    glm::vec3 skyboxRotation = glm::vec3(0.0f);

    std::unordered_map<std::string, Model*> modelCache;

    bool loadFromJSON(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open scene file: " << filepath << std::endl;
            return false;
        }

        json data;
        file >> data; 

        if (data.contains("environment")) {
            if (data["environment"].contains("skyboxPath")) {
                skyboxPath = data["environment"]["skyboxPath"].get<std::vector<std::string>>();
            } else {
                std::string skybox = data["environment"].value("skybox", "");
                if (!skybox.empty()) {
                    skyboxPath.push_back(skybox);
                }
            }
            exposure = data["environment"].value("exposure", 1.0f);
            globalAmbient = data["environment"].value("globalAmbient", 0.08f);
            skyboxRotation = parseVec3(data["environment"].value("skyboxRotation", std::vector<float>{0.0f, 0.0f, 0.0f}));
        }

        if (data.contains("pointLights")) {
            for (const auto& lightNode : data["pointLights"]) {
                PointLightData light;
                light.position = parseVec3(lightNode["position"]);
                
                light.color = parseVec3(lightNode["color"]) / 255.0f;
                
                light.intensity = lightNode.value("intensity", 1.0f);
                pointLights.push_back(light);
            }
        }

        if (data.contains("directionalLight")) {
            dirLight.position = parseVec3(data["directionalLight"]["position"]);
            
            dirLight.color = parseVec3(data["directionalLight"]["color"]) / 255.0f;
            
            dirLight.intensity = data["directionalLight"].value("intensity", 1.0f);
        }

        if (data.contains("entities")) {
            for (const auto& entityNode : data["entities"]) {
                Entity entity;
                entity.name = entityNode.value("name", "unnamed");
                
                entity.transform.position = parseVec3(entityNode["position"]);
                entity.transform.rotation = parseVec3(entityNode["rotation"]);
                
                if (entityNode.contains("rotationVelocity")) {
                    entity.transform.rotationVelocity = parseVec3(entityNode["rotationVelocity"]);
                }

                entity.transform.scale = parseVec3(entityNode["scale"]);

                std::string modelPath = entityNode["modelPath"];
                if (modelCache.find(modelPath) == modelCache.end()) {
                    std::cout << "Loading model: " << modelPath << std::endl;
                    modelCache[modelPath] = new Model(modelPath);
                }
                entity.model = modelCache[modelPath];

                entities.push_back(entity);
            }
        }

        return true;
    }

    void update(float deltaTime) {
        for (auto& entity : entities) {
            entity.transform.rotation += entity.transform.rotationVelocity * deltaTime;
        }
    }

    void cleanUp() {
        for (auto& pair : modelCache) {
            delete pair.second;
        }
        modelCache.clear();
        entities.clear();
        pointLights.clear();
    }
};