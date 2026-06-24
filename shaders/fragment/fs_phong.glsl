#version 460 core

#define MAX_POINT_LIGHTS 10

in vec2 vTexCoords;
in vec3 vNormal;
in vec3 vFragPos;
in vec4 vFragPosLightSpace;
in mat3 vTBN;

out vec4 FragColor;

uniform int numPointLights;
uniform vec3 viewPos;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_emission1;
uniform vec3 material_diffuseColor;
uniform bool hasDiffuseTexture;
uniform bool hasSpecularTexture;
uniform bool hasNormalTexture;
uniform bool normalToggle;
uniform sampler2D shadowMap;
uniform samplerCube shadowCubemaps[MAX_POINT_LIGHTS];
uniform bool hasEmission;
uniform float shininess;
uniform float transparency = 1.0f;
uniform float far_plane;

const vec3 sampleOffsetDirections[20] = vec3[]
(
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

struct DirLight {
    vec3 direction;
    vec3 color;
};
uniform DirLight dirLight;

struct PointLight {
    vec3 position;
    vec3 color;
    float quadratic;
};
uniform PointLight pointLights[MAX_POINT_LIGHTS];

uniform float globalAmbient;

float calcDirShadow(vec4 fragPosLightSpace, vec3 lightDir, vec3 normal)
{
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5f + 0.5f;
	if (projCoords.z > 1.0f) // note: I had this as .x for some reason?
		return 0.0f;
	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;
	float bias = max(0.001f * (1.0f - dot(normal, -lightDir)), 0.001f);
	float shadow = 0.0f;
	vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
	for(int x = -2; x <= 2; ++x)
	{
		for(int y = -2; y <= 2; ++y)
		{
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0f : 0.0f;
		}
	}
	shadow /= 25.0f;
	return shadow;
}

float calcPointShadow(vec3 fragPos, vec3 lightPos, vec3 normal, int lightIndex)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float bias = 0.01f; 
    float shadow = 0.0f;
    float diskRadius = 0.05f;
    int samples = 16;

    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(shadowCubemaps[lightIndex], fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane; // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0f;
    }
    shadow /= float(samples);

    return shadow;
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffuseTex, vec3 specularTex)
{
	// Ambient
    vec3 ambient = globalAmbient * diffuseTex;

    // Diffuse
    vec3 lightDir = normalize(light.direction);
    float diffStrength = max(dot(normal, -lightDir), 0.0f);

    // Specular
    vec3 halfwayDir = normalize(-light.direction + viewDir);
    float specStrength = pow(max(dot(normal, halfwayDir), 0.0f), shininess);

	// Shadow
	float shadow = calcDirShadow(vFragPosLightSpace, lightDir, normal);

    // Total
	vec3 incomingLight = light.color * (1.0f - shadow);
	vec3 diffuseResult = incomingLight * diffStrength * diffuseTex;
	vec3 specularResult = incomingLight * specStrength * specularTex;

	return ambient + diffuseResult + specularResult;

}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseTex, vec3 specularTex, int lightIndex)
{
    // Diffuse
    vec3 lightDir = normalize(light.position - fragPos);
    float diffStrength = max(dot(normal, lightDir), 0.0f);
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specStrength = pow(max(dot(normal, halfwayDir), 0.0f), shininess);
    
    // Attenuation (physical)
    float distance = length(light.position - fragPos);
    float attenuation = 1.0f / ((distance * distance) + 1.0f);

	// Early exit for performance
	vec3 litContribution = light.color * attenuation;
	if (attenuation < 0.001f)
	{
		return vec3(0.0f);
	}
    
    // Shadow
    float shadow = calcPointShadow(fragPos, light.position, normal, lightIndex);
    
    // Total
	vec3 incomingLight = litContribution * (1.0f - shadow);
    vec3 diffuseResult = incomingLight * diffStrength * diffuseTex;
    vec3 specularResult = incomingLight * specStrength * specularTex;

    return diffuseResult + specularResult;
}

void main()
{
	// Properties
	vec3 viewDir = normalize(viewPos - vFragPos);
	vec3 normal;
	if (hasNormalTexture && normalToggle) {
		vec3 normalMap = texture(texture_normal1, vTexCoords).rgb;
		vec3 unpackedvNormal = normalMap * 2.0 - 1.0;
		normal = normalize(vTBN * unpackedvNormal);
	} else {
		normal = normalize(vNormal);
	}
    vec3 diffuseTex = hasDiffuseTexture ? vec3(texture(texture_diffuse1, vTexCoords)) : material_diffuseColor;
	vec3 specularTex = hasSpecularTexture ? vec3(texture(texture_specular1, vTexCoords)) : vec3(0.04f);
	vec3 emissionTex = vec3(texture(texture_emission1, vTexCoords));

	// Lighting calculations
	vec3 result = calcDirLight(dirLight, normal, viewDir, diffuseTex, specularTex);
	// vec3 result = vec3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < numPointLights; i++)
	{
		result += calcPointLight(pointLights[i], normal, vFragPos, viewDir, diffuseTex, specularTex, i);
	}
	// Output
    if (hasEmission) {
        result += emissionTex;
    }
    FragColor = vec4(result, transparency);	
}
