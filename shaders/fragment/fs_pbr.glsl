#version 450 core

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
uniform sampler2D texture_metallic1;
uniform sampler2D texture_roughness1;
uniform sampler2D texture_ao1;
uniform bool hasEmbeddedTextures;
uniform vec3 material_diffuseColor;
uniform bool hasDiffuseTexture;
uniform bool hasSpecularTexture;
uniform bool hasNormalTexture;
uniform bool hasEmissionTexture;
uniform bool hasMetallicTexture;
uniform bool hasRoughnessTexture;
uniform bool hasAOTexture;
uniform bool normalToggle;
uniform bool iblToggle;
uniform sampler2D shadowMap;
uniform samplerCube shadowCubemaps[MAX_POINT_LIGHTS];
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform float shininess;
uniform float transparency = 1.0f;
uniform float far_plane;
uniform float material_roughness;
uniform float material_metallic;

const vec3 sampleOffsetDirections[20] = vec3[]
(
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

struct DirLight {
    vec3 position;
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
	if (projCoords.z > 1.0f || projCoords.x > 1.0f || projCoords.x < 0.0f || projCoords.y > 1.0f || projCoords.y < 0.0f)
		return 1.0f; // shadow everything outside the light's frustum
	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;
	// float bias = max(0.001f * (1.0f - dot(normal, lightDir)), 0.0001f);
	float shadow = 0.0f;
	vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
	for(int x = -2; x <= 2; ++x)
	{
		for(int y = -2; y <= 2; ++y)
		{
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth > pcfDepth ? 1.0f : 0.0f;
		}
	}
	shadow /= 25.0f;
	return shadow;
}

float calcPointShadow(vec3 fragPos, vec3 lightPos, vec3 normal, int lightIndex)
{
    vec3 fragToLight = lightPos - fragPos;
    float currentDepth = length(fragToLight);
    float bias = 0.05f; 
    float shadow = 0.0f;
    float diskRadius = 0.05f;
    int samples = 16;

    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(shadowCubemaps[lightIndex], -fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane; // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0f;
    }
    shadow /= float(samples);

    return shadow;
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

float distributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = 3.14159265359f * denom * denom;

	return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0f);
	float k = (r * r) / 8.0f;

	float num = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx2 = geometrySchlickGGX(NdotV, roughness);
	float ggx1 = geometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo, float metallic, float roughness, vec3 F0)
{
	if (length(light.position) < 0.0001f) return vec3(0.0f);
	vec3 lightDir = normalize(light.position);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	vec3 F = fresnelSchlickRoughness(max(dot(halfwayDir, viewDir), 0.0f), F0, roughness);
	float NDF = distributionGGX(normal, halfwayDir, roughness);
	float G = geometrySmith(normal, viewDir, lightDir, roughness);
	vec3 numerator = NDF * G * F;
	float denominator = 4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDir), 0.0f) + 0.0001f;
	vec3 specular = numerator / denominator;
	vec3 kS = F;
	vec3 kD = vec3(1.0f) - kS;
	kD *= 1.0f - metallic;

	float shadow = calcDirShadow(vFragPosLightSpace, lightDir, normal);
	vec3 radiance = light.color * (1.0f - shadow);
	
	float NdotL = max(dot(normal, lightDir), 0.0f);
	return (kD * albedo / 3.14159265359f + specular) * radiance * NdotL;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 albedo, float metallic, float roughness, vec3 F0, int lightIndex)
{
	if (length(light.position) < 0.0001f) return vec3(0.0f);
	vec3 lightDir = normalize(light.position - vFragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float distance = length(light.position - vFragPos);
	float attenuation = 1.0f / (distance * distance);

	// Early exit for performance
	vec3 litContribution = light.color * attenuation;
	if (attenuation < 0.001f)
	{
		return vec3(0.0f);
	}

	vec3 radiance = light.color * attenuation;
	vec3 F = fresnelSchlickRoughness(max(dot(halfwayDir, viewDir), 0.0f), vec3(F0), roughness);
	float NDF = distributionGGX(normal, halfwayDir, roughness);
	float G = geometrySmith(normal, viewDir, lightDir, roughness);
	vec3 numerator = NDF * G * F;
	float denominator = 4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDir), 0.0f) + 0.0001f;
	vec3 specular = numerator / denominator;
	vec3 kS = F;
	vec3 kD = vec3(1.0f) - kS;
	kD *= 1.0f - metallic;
    
    // Shadow
    float shadow = calcPointShadow(vFragPos, light.position, normal, lightIndex);

	radiance = radiance * (1.0f - shadow);
	
	float NdotL = max(dot(normal, lightDir), 0.0f);
	return (kD * albedo / 3.14159265359f + specular) * radiance * NdotL;
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
    vec4 albedoTex = hasDiffuseTexture ? texture(texture_diffuse1, vTexCoords) : vec4(material_diffuseColor, 1.0f);
	vec3 albedo = albedoTex.rgb;

    float alpha = transparency;
	if (alpha < 0.05f) {
		discard;
	}

	vec3 emissionTex = hasEmissionTexture ? texture(texture_emission1, vTexCoords).rgb : vec3(0.0f);

    float roughness = material_roughness;
    float metallic = material_metallic;
    float ao = 1.0f;

    if (hasEmbeddedTextures) {
        // glTF PBR standard: Green = Roughness, Blue = Metallic, Red = AO
        if (hasRoughnessTexture) roughness = texture(texture_roughness1, vTexCoords).g;
		if (hasMetallicTexture) metallic = texture(texture_metallic1, vTexCoords).b;
        if (hasAOTexture) {
			float rawAO = texture(texture_ao1, vTexCoords).r;
			ao = (rawAO < 0.01f) ? 1.0f : rawAO; 
		}
    } else {
        // Standard discrete textures: scalar values in Red channel
        if (hasRoughnessTexture) roughness = texture(texture_roughness1, vTexCoords).r;
		if (hasMetallicTexture) metallic = texture(texture_metallic1, vTexCoords).r;
		if (hasAOTexture) {
			float rawAO = texture(texture_ao1, vTexCoords).r;
			ao = (rawAO < 0.01f) ? 1.0f : rawAO; 
		}
    }

	vec3 F0 = vec3(0.04f); // Default reflectance for dielectrics
    
    // Specular vs Metallic workflow for F0
    if (hasSpecularTexture && !hasMetallicTexture && !hasEmbeddedTextures) {
        vec3 specColor = texture(texture_specular1, vTexCoords).rgb;
        F0 = specColor;
        metallic = (max(specColor.r, max(specColor.g, specColor.b)) > 0.1f) ? 1.0f : 0.0f;
    } else {
        // Metallic workflow (using the safely extracted metallic value from above)
        if (hasSpecularTexture) {
            float specFactor = texture(texture_specular1, vTexCoords).r;
            F0 = mix(vec3(0.08f * specFactor), albedo, metallic);
        } else {
            F0 = mix(vec3(0.04f), albedo, metallic);
        }
    }

	// Lighting calculations
	vec3 Lo = vec3(0.0f);
	Lo += calcDirLight(dirLight, normal, viewDir, albedo, metallic, roughness, F0);
	// vec3 result = vec3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < numPointLights; i++)
	{
		Lo += calcPointLight(pointLights[i], normal, viewDir, albedo, metallic, roughness, F0, i);
	}
	// Output
    if (hasEmissionTexture) {
        Lo += emissionTex;
    }

	// Ambient
	if (iblToggle) {
		vec3 kS = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness); 
		vec3 kD = 1.0 - kS;
		kD *= 1.0 - metallic;

		// Diffuse IBL
		vec3 irradiance = texture(irradianceMap, normal).rgb;
		vec3 diffuse = irradiance * albedo;

		// Specular IBL
		const float MAX_REFLECTION_LOD = 4.0;
		vec3 R = reflect(-viewDir, normal);
		vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
		vec2 envBRDF = texture(brdfLUT, vec2(max(dot(normal, viewDir), 0.0), roughness)).rg;
		vec3 specular = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);

		vec3 ambient = (kD * diffuse + specular) * ao; 
		Lo += ambient;
	} else {
		vec3 ambient = globalAmbient * albedo * ao;
		Lo += ambient;
	}

    FragColor = vec4(Lo, alpha);	
}
