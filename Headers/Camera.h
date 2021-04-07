#ifndef HEADERS_CAMERAHANDLER_H_
#define HEADERS_CAMERAHANDLER_H_

#include <glm/gtx/rotate_vector.hpp>
#include <glm/glm.hpp>

class Camera
{
    public:
        Camera(void);
        ~Camera(void);

        void moveLeft(void);
        void moveRight(void);
        void moveUp(void);
        void moveDown(void);
        void moveForward(void);
        void moveBackward(void);

        void rotateUp(void);
        void rotateDown(void);
        void rotateLeft(void);
        void rotateRight(void);

        // Returns vec3 position
        glm::vec3 getPosition(void);
        // returns vec3 that represents
        // a front facing vector
        glm::vec3 getFront(void);

    private:
        glm::vec3 position;
        glm::vec3 frontFace;
        glm::vec3 rotationAngle;
};

#endif