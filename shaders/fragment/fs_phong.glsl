#version 460 core

#define MAX_POINT_LIGHTS 10

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform int numPointLights;
uniform vec3 viewPos;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_emission1;
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

// struct Material {
//     sampler2D diffuse;
//     sampler2D specular;
//     float shininess;
// };
// uniform Material material;

struct DirLight {
	vec3 color;
	vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

struct PointLight {
	vec3 color;
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constant;
	float linear;
	float quadratic;
};
uniform PointLight pointLights[MAX_POINT_LIGHTS];

float calcDirShadow(vec4 fragPosLightSpace, vec3 lightDir, vec3 normal)
{
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5f + 0.5f;
	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;
	float bias = max(0.001f * (1.0f - dot(normal, -lightDir)), 0.0001f);
	float shadow = 0.0f;
	vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
	for(int x = -2; x <= 2; ++x)
	{
		for(int y = -2; y <= 2; ++y)
		{
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) *texelSize).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0f : 0.0f;
		}
	}
	shadow /= 25.0f;
	if (projCoords.z > 1.0f) // note: I had this as .x for some reason?
		shadow = 0.0f;
	return shadow;
}

float calcPointShadow(vec3 fragPos, vec3 lightPos, vec3 normal, int lightIndex)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float bias = 0.01f; 
    float shadow = 0.0f;
    float diskRadius = 0.05f;
    int samples = 20;

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
    vec3 ambient = light.ambient * diffuseTex;

    // Diffuse
    vec3 lightDir = normalize(light.direction);
    float diffStrength = max(dot(normal, -lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diffStrength * diffuseTex;

    // Specular
    vec3 halfwayDir = normalize(-light.direction + viewDir);
    float specStrength = pow(max(dot(normal, halfwayDir), 0.0f), shininess);
	vec3 specular = light.specular * specStrength * specularTex;

    // Total
	float shadow = calcDirShadow(FragPosLightSpace, lightDir, normal);
    return light.color * ambient + (1.0f - shadow) * (diffuse + specular);
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseTex, vec3 specularTex, int lightIndex)
{
	// Ambient
	vec3 ambient = light.ambient * diffuseTex;

	// Diffuse
	vec3 lightDir = normalize(fragPos - light.position);
	float diffStrength = max(dot(normal, -lightDir), 0.0f);
	vec3 diffuse = light.diffuse * diffStrength * diffuseTex;

	// Specular
	vec3 halfwayDir = normalize(-lightDir + viewDir);
	float specStrength = pow(max(dot(normal, halfwayDir), 0.0f), shininess);
	vec3 specular = light.specular * specStrength * specularTex;
	
	// Attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0f / (light.constant + light.linear * distance +
		light.quadratic * (distance * distance));
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	// Total
	if (attenuation < 0.01f) { // avoid unnecessary shadow calculations
		return light.color * (ambient + diffuse + specular);
	}; 
	float shadow = calcPointShadow(fragPos, light.position, normal, lightIndex);
	return light.color * ambient + (1.0f - shadow) * (diffuse + specular);
	// return light.color * ambient + (diffuse + specular);

}

void main()
{
	// Properties
	vec3 normal = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 diffuseTex = vec3(texture(texture_diffuse1, TexCoords));
	vec3 specularTex = vec3(texture(texture_specular1, TexCoords));
	vec3 emissionTex = vec3(texture(texture_emission1, TexCoords));

	// Lighting calculations
	vec3 result = calcDirLight(dirLight, normal, viewDir, diffuseTex, specularTex);
	// vec3 result = vec3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < numPointLights; i++)
	{
		result += calcPointLight(pointLights[i], normal, FragPos, viewDir, diffuseTex, specularTex, i);
	}
	// Output
    if (hasEmission) {
        result += vec3(texture(texture_emission1, TexCoords));
    }
    FragColor = vec4(result, transparency);	
}
