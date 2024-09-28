#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float uv_x;
layout(location = 2) in vec3 color;
layout(location = 3) in float uv_y;
layout(location = 4) in vec3 normal;
layout(location = 5) in float inTexIndex;
layout(location = 6) in int EntityID;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out float frag_uv_x;
layout(location = 2) out vec3 fragPosWorld;
layout(location = 3) out float frag_uv_y;
layout(location = 4) out vec3 fragNormalWorld;
layout(location = 5) out float fragInTexIndex;
layout(location = 6) out flat int fragEntityID;

struct PointLight
	{
		vec4 position;
		vec4 color;
	};

layout(set = 0, binding = 0) uniform GlobalUbo{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 inverseViewMatrix;
	vec4 ambientLightCol;
	PointLight pointLights[10];
	int numLights;
} ubo;

struct ObjectModelData{
	mat4 modelMatrix;
	mat4 normalMatrix; 
};

layout(std140,set = 0, binding = 2) readonly buffer ObjectBuffer{
	ObjectModelData objects[];
} objectBuffer;

void main() {
	vec4 worldPos = objectBuffer.objects[EntityID].modelMatrix * vec4(inPosition,1.0);
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix  * worldPos;
	
	fragNormalWorld = normalize(mat3(objectBuffer.objects[EntityID].normalMatrix)*normal);
	fragPosWorld = worldPos.xyz;
	fragColor = color;
	frag_uv_x = uv_x;
	frag_uv_y = uv_y;
	fragInTexIndex = inTexIndex;
	fragEntityID = EntityID;
}