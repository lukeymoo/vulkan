#include "Camera.h"

Camera::Camera(void)
{
    position = {0.0f, 0.0f, -2.0f};
    // 1 unit forward z axis default front face
    // later mouse will control camera angles
    frontFace = {0.0f, 0.0f, 1.0f};

    rotationAngle = {0.0f, 0.0f, 0.0f};
    return;
}

Camera::~Camera(void)
{
    return;
}

glm::vec3 Camera::getFront(void)
{
    glm::vec3 front(1.0);
    glm::vec4 temp(1.0);

    glm::mat4 rotMatrixX(1.0);
    glm::mat4 rotMatrixY(1.0);
    glm::mat4 rotMatrixZ(1.0);

    rotMatrixX = glm::rotate(rotMatrixX, rotationAngle.x, glm::vec3(1.0f, 0.0f, 0.0f));
    temp = temp * rotMatrixX;
    rotMatrixY = glm::rotate(rotMatrixY, rotationAngle.y, glm::vec3(0.0f, 1.0f, 0.0f));
    temp = temp * rotMatrixY;
    rotMatrixZ = glm::rotate(rotMatrixZ, rotationAngle.z, glm::vec3(0.0f, 0.0f, 1.0f));
    temp = temp * rotMatrixZ;

    front.x = temp.x;
    front.y = temp.y;
    front.z = temp.z;

    return front;
}

glm::vec3 Camera::getPosition(void)
{
    return position;
}

void Camera::moveForward(void)
{
    position.z += 0.2f;
    return;
}

void Camera::moveBackward(void)
{
    position.z -= 0.2f;
    return;
}

void Camera::moveLeft(void)
{
    position.x -= 0.2f;
    return;
}

void Camera::moveRight(void)
{
    position.x += 0.2f;
    return;
}

void Camera::moveUp(void)
{
    position.y -= 0.2f;
    return;
}

void Camera::moveDown(void)
{
    position.y += 0.2f;
    return;
}

void Camera::rotateUp(void)
{
    rotationAngle.z -= 0.2f;
    return;
}

void Camera::rotateDown(void)
{
    rotationAngle.z += 0.2f;
    return;
}

void Camera::rotateLeft(void)
{
    rotationAngle.y -= 0.2f;
    return;
}

void Camera::rotateRight(void)
{
    rotationAngle.y += 0.2f;
    return;
}