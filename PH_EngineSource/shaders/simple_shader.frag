#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in float frag_uv_x;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in float frag_uv_y;
layout(location = 4) in vec3 fragNormalWorld;
layout(location = 5) in float fragInTexIndex;
layout(location = 6) in flat int fragEntityID;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

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

struct ObjectModelData{
	mat4 modelMatrix;
	mat4 normalMatrix; 
};

layout(std140,set = 0, binding = 2) readonly buffer ObjectBuffer{
	ObjectModelData objects[];
} objectBuffer;

void main() {
	vec3 diffuseLight = ubo.ambientLightCol.xyz * ubo.ambientLightCol.w;
	vec3 specularLight = vec3(0.0);
	vec3 surfaceNormal = normalize(fragNormalWorld);
		
	vec3 cameraPosWorld = ubo.inverseViewMatrix[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

	for(int i =0 ; i < ubo.numPointLights; i++)
	{
		PointLight light = ubo.pointLights[i];
		vec3 directionToLight = light.position.xyz - fragPosWorld;
		float attenuation =1.0 / dot(directionToLight,directionToLight);
		directionToLight = normalize(directionToLight);
		float cosAngIncidence = max(dot(surfaceNormal,directionToLight),0);
		vec3 intensity = light.color.xyz * light.color.w * attenuation;

		diffuseLight += intensity * cosAngIncidence;
		
		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm,0,1);
		blinnTerm = pow(blinnTerm,512.0);
		specularLight +=intensity * blinnTerm;

	}
	vec4 difColor = vec4(diffuseLight*fragColor + specularLight * fragColor, 1.0);
	outColor = texture(texSampler, vec2(frag_uv_x,frag_uv_y)) + difColor;
}