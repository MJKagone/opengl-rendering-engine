#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat3 normalMatrix;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 dirLightSpaceMatrix;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0f));
    Normal = mat3(normalMatrix) * aNormal;
	TexCoords = aTexCoords;
    FragPosLightSpace  = dirLightSpaceMatrix * vec4(FragPos, 1.0f);
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
