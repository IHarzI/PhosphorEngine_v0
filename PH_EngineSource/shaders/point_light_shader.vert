#version 460

const vec2 OFFSETS[6] = vec2[](
vec2(-1.0,-1.0),
vec2(-1.0,1.0),
vec2(1.0,-1.0),
vec2(1.0,-1.0),
vec2(-1.0,1.0),
vec2(1.0,1.0));

layout (location = 0) out vec2 fragOffset;

struct PointLight
	{
		vec4 position;
		vec4 color;
	};

	struct DirectionalLight
	{
		vec4 position;
		vec4 color;
		vec3 shadowExtent;
		vec3 direction;
	};

layout(std140, set = 0, binding = 0) uniform GlobalUbo{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 inverseViewMatrix;
	mat4 DirectionalShadowMatrix;
	mat4 MainLightProjection;
	mat4 MainLightView;
	vec4 ambientLightCol;
	int numPointLights;
	int numDirectionalLights;
	float DirectionalLightShadowCast;
	PointLight pointLights[10];
	DirectionalLight directionalLights[5];
} ubo;

layout (push_constant) uniform Push{
	vec4 position;
	vec4 color;
	float radius;
} push;

void main()
{
	fragOffset = OFFSETS[gl_VertexIndex];

	vec3 cameraRightWorld = {ubo.viewMatrix[0][0], ubo.viewMatrix[1][0], ubo.viewMatrix[2][0]};
	vec3 cameraUpWorld = {ubo.viewMatrix[0][1], ubo.viewMatrix[1][1], ubo.viewMatrix[2][1]};

	vec3 positionWorld = push.position.xyz + push.radius * fragOffset.x * cameraRightWorld
	+ push.radius * fragOffset.y * cameraUpWorld;

	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(positionWorld, 1.0);
} 