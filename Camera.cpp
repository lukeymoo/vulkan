#include "Camera.h"

Camera::Camera(int extentWidth, int extentHeight)
    : viewWidth(extentWidth), viewHeight(extentHeight)
{
    viewMatrix = glm::mat4(1.0);
    projMatrix = glm::mat4(1.0);

    lookAt = DEFAULT_FORWARD_VECTOR;

    position = {0.0f, -3.0f, -8.0f};
    return;
}

Camera::~Camera(void)
{
    return;
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

void Camera::update(void)
{
    return;
}

void Camera::rotate(glm::vec3 rotateBy)
{
    return;
}

glm::vec3 Camera::getPosition(void)
{
    return position;
}