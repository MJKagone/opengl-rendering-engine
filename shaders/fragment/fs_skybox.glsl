#version 450 core

in vec3 vTexCoords;
out vec4 FragColor;

uniform samplerCube skybox;

void main()
{
    vec3 color = texture(skybox, vTexCoords).rgb;
    FragColor = vec4(color, 1.0);
}