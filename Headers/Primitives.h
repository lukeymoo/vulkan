#ifndef HEADERS_PRIMITIVES_H_
#define HEADERS_PRIMITIVES_H_

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <array>
#include <vector>
#include <chrono>

struct Vertex
{
	glm::vec4 pos;
	glm::vec4 color;

	static std::vector<VkVertexInputBindingDescription> getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<VkVertexInputBindingDescription> bindingDescs = {bindingDescription};

		return bindingDescs;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		VkVertexInputAttributeDescription position{};
		VkVertexInputAttributeDescription color{};

		// Position
		position.binding = 0;
		position.location = 0;
		position.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		position.offset = offsetof(Vertex, pos);

		// Color
		color.binding = 0;
		color.location = 1;
		color.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		color.offset = offsetof(Vertex, color);

		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
			position, color};

		return attributeDescriptions;
	}
};

// Binding = 0
struct UniformModelBuffer
{
	alignas(16) glm::mat4 model;
};

// Binding = 1
struct UniformVPBuffer
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

#endif
