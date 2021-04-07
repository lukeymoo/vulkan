#version 450
#extension GL_ARB_separate_shader_objects : enable

/*
	Whenever a new model is being drawn the buffer
	will be rebound in the CommandBuffer at a new offset

	The shader only needs to grab 64 bytes per vertex and apply the math

	CPU side code will handle putting the correct 64 bytes in front of the shader
*/
layout(binding = 0) uniform uniformModelBuffer {
	mat4 model;
} m;

layout(binding = 1) uniform viewProjectionBuffer {
	mat4 view;
	mat4 proj;
} vp;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

// Will add a uniform buffer that will contain View and project matrices
// 128 bytes of data should fit into a push constant for high speed updates of camera view


/* OUTPUT */
layout(location = 0) out vec4 fragColor;

struct testTriangle {
	vec3 pos;
	vec4 color;
};

vec3 hp[3] = {
	{-0.5f, -0.5f, 0.0f},
	{ 0.5f, -0.5f, 0.0f},
	{ 0.5f,  0.5f, 0.0f}
};

vec4 colors[3] = {
	{1.0f, 0.0f, 0.0f, 1.0f},
	{0.0f, 1.0f, 0.0f, 1.0f},
	{0.0f, 0.0f, 1.0f, 1.0f}
};

void main() {
	gl_Position = vp.proj * vp.view * m.model * vec4(inPosition, 1.0);
	// if(inPosition[0] == 0 || inPosition[1] == 0 || inPosition[2] == 0) {
	// 	gl_Position = vec4(hp[gl_VertexIndex], 1.0);
	// } else {
	// 	gl_Position = vec4(inPosition, 1.0);
	// }
	// passthru
	fragColor = inColor;
}
