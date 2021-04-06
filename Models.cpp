#include "Models.h"

HumanClass::HumanClass(void) {
    return;
}

HumanClass::~HumanClass(void) {
    return;
}

ModelClass::ModelClass(void) {
    typeName = "[Undefined Model Type!]";
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
std::pair<int, int> ModelClass::loadModelData(std::pair<int, int> vertexAndIndexOffsets, std::string modelTypeName) {
    typeName = modelTypeName;
    // Store the given start location
    vertexStartOffset = vertexAndIndexOffsets.first;
    indexStartOffset = vertexAndIndexOffsets.second;

    // Populate local vertex container
    vertices = {
          {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
          {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
          {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
          {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };

    indices = {
        0, 1, 2, 2, 3, 0
    };

    // We know we cannot exceed INITIAL_BUFFER_SIZE so lets ensure
    // our model data will not do so
    int vertexEndOffset = vertexStartOffset + (vertices.size() * sizeof(Vertex));
    int indexEndOffset = indexStartOffset + (indices.size() * sizeof(uint16_t));

    vertexDataSize = sizeof(Vertex) * vertices.size();
    indexDataSize = sizeof(uint16_t) * indices.size();

    if(vertexEndOffset> VERTEX_BUFFER_SIZE) {
        std::cout << "\t[-] Self check : Model data will exceed vertex buffer size" << std::endl;
        return { -1, -1 };
    } else {
        std::cout << "\t[+] Self check : Model data within memory constraints" << std::endl;
    }

    if(indexEndOffset > INDEX_BUFFER_SIZE) {
        std::cout << "\t[-] Self check : Model index data will exceed index buffer size" << std::endl;
        return { -1, -1 };
    }
    return { vertexEndOffset, indexEndOffset };
}
