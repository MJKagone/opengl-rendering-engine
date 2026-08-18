#version 450 core

in vec2 vTexCoords;
out vec4 FragColor;
uniform sampler2D depthMap;

void main()
{
    float depthValue = texture(depthMap, vTexCoords).r;
    FragColor = vec4(vec3(depthValue), 1.0);
}
