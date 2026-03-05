#version 330 core

#define NR_POINT_LIGHTS 2

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 viewPos;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};
uniform Material material;

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
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
	// Ambient
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));

    // Diffuse
    vec3 lightDir = normalize(light.direction);
    float diffStrength = max(dot(normal, -lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diffStrength * vec3(texture(material.diffuse, TexCoords));

    // Specular
    vec3 reflectDir = reflect(lightDir, normal);
    float specStrength = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
    vec3 specular = light.specular * specStrength *
        vec3(texture(material.specular, TexCoords));

    // Total
    return light.color * (ambient + diffuse + specular);
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	// Ambient
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));

	// Diffuse
	vec3 lightDir = normalize(fragPos - light.position);
	float diffStrength = max(dot(normal, -lightDir), 0.0f);
	vec3 diffuse = light.diffuse * diffStrength * vec3(texture(material.diffuse, TexCoords));

	// Specular
	vec3 reflectDir = reflect(lightDir, normal);
	float specStrength = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
	vec3 specular = light.specular * specStrength *
		vec3(texture(material.specular, TexCoords));
	
	// Attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance +
		light.quadratic * (distance * distance));
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	return light.color * (ambient + diffuse + specular);
}

void main()
{
	// Properties
	vec3 normal = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);

	// Lighting calculations
	//vec3 result = calcDirLight(dirLight, normal, viewDir);
	vec3 result = vec3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < NR_POINT_LIGHTS; i++)
	{
		result += calcPointLight(pointLights[i], normal, FragPos, viewDir);
	}

    // Output
    FragColor = vec4(result, 1.0f);
}
