#ifndef HEADERS_CAMERAHANDLER_H_
#define HEADERS_CAMERAHANDLER_H_

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

class Camera
{
public:
    Camera(int extentWidth, int extentHeight);
    ~Camera(void);

    void moveLeft(void);
    void moveRight(void);
    void moveUp(void);
    void moveDown(void);
    void moveForward(void);
    void moveBackward(void);

    void rotate(glm::vec3 rotateBy);

    void update(void);

    glm::vec3 getPosition(void);

private:
    int viewWidth;
    int viewHeight;

    float fovDegrees = 90.0f; // field of view
    float fovRadians = (fovDegrees / 360.0f) / (glm::two_pi<float>());
    float aspectRatio = static_cast<float>(viewWidth) / static_cast<float>(viewHeight);
    float nearZ = 0.01f;
    float farZ = 1000.0f;


    // xyz coords
    glm::vec3 position;
    // Changes based on angle of camera
    glm::vec3 upVector;
    // Origin of camera focus
    glm::vec3 lookAt;
    // rotation vector
    glm::vec3 rotVector;

    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;

public:
    /* CONSTANTS */
    const glm::vec3 DEFAULT_UP_VECTOR = {0.0f, -1.0f, 0.0f};
    const glm::vec3 DEFAULT_DOWN_VECTOR = {0.0f, 1.0f, 0.0f};
    const glm::vec3 DEFAULT_FORWARD_VECTOR = {0.0f, 0.0f, 1.0f};
    const glm::vec3 DEFAULT_BACKWARD_VECTOR = {0.0f, 0.0f, -1.0f};
    const glm::vec3 DEFAULT_RIGHT_VECTOR = {1.0f, 0.0f, 0.0f};
    const glm::vec3 DEFAULT_LEFT_VECTOR = {-1.0f, 0.0f, 0.0f};
};

#endif