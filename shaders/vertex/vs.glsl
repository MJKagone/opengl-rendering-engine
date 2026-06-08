#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out mat3 TBN;

uniform mat4 model;
uniform mat3 normalMatrix;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 dirLightSpaceMatrix;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0f));
    Normal = mat3(normalMatrix) * aNormal;
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    TBN = mat3(T, B, N);
	TexCoords = aTexCoords;
    FragPosLightSpace  = dirLightSpaceMatrix * vec4(FragPos, 1.0f);
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
