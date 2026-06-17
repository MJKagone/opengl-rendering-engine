#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 vNormal;
out vec3 vFragPos;
out vec2 vTexCoords;
out vec4 vFragPosLightSpace;
out mat3 vTBN;

uniform mat4 model;
uniform mat3 normalMatrix;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 dirLightSpaceMatrix;

void main()
{
    vFragPos = vec3(model * vec4(aPos, 1.0f));
    vNormal = mat3(normalMatrix) * aNormal;
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    vTBN = mat3(T, B, N);
	vTexCoords = aTexCoords;
    vFragPosLightSpace  = dirLightSpaceMatrix * vec4(vFragPos, 1.0f);
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
