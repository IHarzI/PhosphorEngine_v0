#version 460

layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;

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

const float M_PI = 3.1415926538;
void main()
{
	float dis = sqrt(dot(fragOffset,fragOffset));

	if(dis > push.radius)
	{
		discard;
	}
	outColor = vec4(push.color.xyz*push.color.w, 0.5*(cos(dis * M_PI)));
}