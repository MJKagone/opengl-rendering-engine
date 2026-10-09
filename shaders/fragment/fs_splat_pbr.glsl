#version 450 core

// PBR appearance for augmented 3DGS plys (R3DG-style: normals, base color,
// roughness, metallic). Same lighting as fs_pbr.glsl (Cook-Torrance + shadows
// + IBL), but material data comes per splat from the vertex shader instead
// of mesh textures. Coverage/blending still uses the gaussian alpha falloff.

#define MAX_POINT_LIGHTS 10

in vec3 vColor;
in vec3 vNormal;
in float vRoughness;
in float vMetallic;
in float vOpacity;
in vec3 vFragPos;
in vec4 vFragPosLightSpace;
in vec2 vCenter;
in vec3 vConic;

out vec4 FragColor;

uniform int numPointLights;
uniform vec3 viewPos;
uniform bool lightToggle;
uniform bool iblToggle;
uniform bool specularIBLOnlyMirror;
uniform float globalAmbient;
uniform float far_plane;
uniform sampler2D shadowMap;
uniform samplerCube shadowCubemaps[MAX_POINT_LIGHTS];
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

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

const vec3 sampleOffsetDirections[20] = vec3[]
(
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float calcDirShadow(vec4 fragPosLightSpace, vec3 lightDir, vec3 normal)
{
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5f + 0.5f;
	if (projCoords.z > 1.0f || projCoords.x > 1.0f || projCoords.x < 0.0f || projCoords.y > 1.0f || projCoords.y < 0.0f)
		return 1.0f; // shadow everything outside the light's frustum
	float currentDepth = projCoords.z;
	float shadow = 0.0f;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for (int x = -2; x <= 2; ++x)
	{
		for (int y = -2; y <= 2; ++y)
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

    for (int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(shadowCubemaps[lightIndex], -fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane; // undo mapping [0;1]
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
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

    float shadow = calcPointShadow(vFragPos, light.position, normal, lightIndex);

	radiance = radiance * (1.0f - shadow);

	float NdotL = max(dot(normal, lightDir), 0.0f);
	return (kD * albedo / 3.14159265359f + specular) * radiance * NdotL;
}

void main()
{
    // Gaussian alpha falloff (same as fs_splat.glsl)
    vec2 d = gl_FragCoord.xy - vCenter;
    float power = -0.5 * (vConic.x * d.x * d.x + vConic.z * d.y * d.y) - vConic.y * d.x * d.y;
    if (power > 0.0) discard;

    float alpha = min(0.99, vOpacity * exp(power));
    if (alpha < 1.0 / 255.0) discard;

    // Material from the splat
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(viewPos - vFragPos);
    vec3 albedo = vColor;
    float roughness = clamp(vRoughness, 0.045, 1.0);
    float metallic = clamp(vMetallic, 0.0, 1.0);
    vec3 F0 = mix(vec3(0.04f), albedo, metallic);

    // Lighting
    vec3 Lo = vec3(0.0f);
    if (lightToggle) {
        Lo += calcDirLight(dirLight, normal, viewDir, albedo, metallic, roughness, F0);
    }
    for (int i = 0; i < numPointLights; i++)
    {
        Lo += calcPointLight(pointLights[i], normal, viewDir, albedo, metallic, roughness, F0, i);
    }

    // Ambient
    if (iblToggle) {
        vec3 kS = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;

        vec3 irradiance = texture(irradianceMap, normal).rgb;
        vec3 diffuse = irradiance * albedo;

        const float MAX_REFLECTION_LOD = 4.0;
        vec3 R = reflect(-viewDir, normal);
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 envBRDF = texture(brdfLUT, vec2(max(dot(normal, viewDir), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);
        if (specularIBLOnlyMirror && !(roughness < 0.01 && metallic > 0.99)) {
            specular = vec3(0.0);
        }

        vec3 ambient = kD * diffuse + specular;
        Lo += ambient;
    } else {
        Lo += globalAmbient * albedo;
    }

    FragColor = vec4(Lo, alpha);
}
