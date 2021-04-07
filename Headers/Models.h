#ifndef __MODELS_H_
#define __MODELS_H_

#include "Primitives.h"

/*
Base class for all objects that contain vertex data
and any textures that need to be loaded

Uses Primitives defined in Primitives.h
*/

#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>

// Total allocation for static vertex and index information
// Data stored in buffers allocated with these constants define MODELS themselves
const int VERTEX_BUFFER_SIZE = 256000000; // 256 MB
const int INDEX_BUFFER_SIZE = 100000000;  // 100 MB
// Max uniform buffer size is defined later
// each graphics device has max which we cannot exceed

class HumanClass
{
public:
  HumanClass(void);
  ~HumanClass(void);

  /* Model matrix */
  // Applied to model TYPE vertex data
  // to achieve a position in WORLD
  glm::vec3 position;
  glm::mat4 worldMatrix;
};

class ModelClass
{
public:
  // Will handle loading model data
  ModelClass(std::string modelTypeName);
  ModelClass(void) = delete;
  ~ModelClass(void);

  // safety
  ModelClass &operator=(const ModelClass &) = delete;

  // These offsets help each object type track it's own
  // location in memory; These offsets can be passed to
  // vulkan binding functions and subsequently the shader
  // Vertex offset for vertex buffer, etc
  int vertexStartOffset = 0;
  int indexStartOffset = 0;

  int vertexDataSize = 0;
  int indexDataSize = 0;
  std::string typeName;

  // Returns offsets for VERTEX, INDEX buffer respectively
  std::pair<int, int> loadModelData(std::pair<int, int> vertexAndIndexBufferOffsets);

  /*
        ** Humans share the same vertex/index data
        **
        ** In HumanClass, individualized data such as
        ** transformation data, color data, texture data etc can be defined
         */
  std::vector<HumanClass> humans;

  // Describe this model TYPE
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;
};

#endif // __MODELS_H_
