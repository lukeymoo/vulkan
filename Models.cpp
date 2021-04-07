#include "Models.h"

HumanClass::HumanClass(void) {
    position = {0.0f, 0.0f, 0.0f};
    worldMatrix = glm::mat4();
    worldMatrix = glm::translate(worldMatrix, position);
    return;
}

HumanClass::~HumanClass(void) {
    return;
}

/*
** Just validates existence of file
 */
ModelClass::ModelClass(std::string modelTypeName) {
    typeName = modelTypeName;
    return;
}

ModelClass::~ModelClass(void) {
    return;
}

/*
** For now will just use predefined vertices
**
** But later will read into specified file and
** validate/fill vertices container with data from file
 */
std::pair<int, int> ModelClass::loadModelData(std::pair<int, int> vertexAndIndexOffsets) {
    // Store the given start location
    vertexStartOffset = vertexAndIndexOffsets.first;
    indexStartOffset = vertexAndIndexOffsets.second;

    // Describe the shape of this model
    // Vertex data per instance is located in subclass container
    vertices = {
            {{-0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
    };

    indices = {
        0, 1, 2, 2, 3, 0
    };

    // We know we cannot exceed INITIAL_BUFFER_SIZE so lets ensure
    // our model data will not do so
    vertexDataSize = sizeof(Vertex) * vertices.size();
    indexDataSize = sizeof(uint16_t) * indices.size();

    int vertexEndOffset = vertexStartOffset + vertexDataSize;
    int indexEndOffset = indexStartOffset + indexDataSize;

    if(vertexEndOffset > VERTEX_BUFFER_SIZE) {
        std::cout << "\t[-] Self check : Model data will exceed vertex buffer size" << std::endl;
        return { -1, -1 };
    }

    if(indexEndOffset > INDEX_BUFFER_SIZE) {
        std::cout << "\t[-] Self check : Model index data will exceed index buffer size" << std::endl;
        return { -1, -1 };
    }
    return { vertexEndOffset, indexEndOffset };
}
