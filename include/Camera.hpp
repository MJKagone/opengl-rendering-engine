#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {

private:
    const float SENSITIVITY = 0.05f;
    const float SPEED = 15.0f;
    
public:
    enum Direction {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 worldUp;

    float yaw = -90.0f;
    float pitch = 0.0f;
    bool firstMouse = true;
    float fov = 45.0f;
    
    Camera(glm::vec3 cameraPos, glm::vec3 cameraFront, glm::vec3 worldUp) {
        this->pos = cameraPos;
        this->front = cameraFront;
        this->worldUp = worldUp;
    }
    
    glm::mat4 getViewMatrix() {
        return glm::lookAt(this->pos, this->pos + this->front, this->worldUp);
    }
    
    void processKeyboard(Direction direction, float deltaTime) {
        if (direction == FORWARD) {pos += this->SPEED * deltaTime * front;}
        if (direction == BACKWARD) {pos -= this->SPEED * deltaTime * front;}
        if (direction == RIGHT) {pos += this->SPEED * deltaTime * glm::normalize(glm::cross(front, worldUp));}
        if (direction == LEFT) {pos -= this->SPEED * deltaTime * glm::normalize(glm::cross(front, worldUp));}
        if (direction == UP) {pos += this->SPEED * deltaTime * worldUp;}
        if (direction == DOWN) {pos -= this->SPEED * deltaTime * worldUp;}
    }
    
    void processMouse(float deltaX, float deltaY) {
        this->yaw += deltaX * this->SENSITIVITY;
        this->pitch += deltaY * this->SENSITIVITY;

        if (this->pitch > 89.0f) {
            this->pitch = 89.0f;
        }
        else if (this->pitch < -89.0f) {
            this->pitch = -89.0f;
        }

        this->front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->front.y = sin(glm::radians(pitch));
        this->front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->front = glm::normalize(this->front);
    }

    void processScroll(float deltaY) {
        this->fov -= 2.0f * deltaY;
        if (this->fov < 1.0f) {this->fov = 1.0f;}
        if (this->fov > 45.0f) {this->fov = 45.0f;}
    }
};