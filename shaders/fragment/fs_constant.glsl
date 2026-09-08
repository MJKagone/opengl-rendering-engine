#version 450 core

in vec2 vTexCoords;
out vec4 FragColor;
uniform sampler2D texture_diffuse1;
uniform bool hasDiffuseTexture;
uniform vec3 material_diffuseColor;

void main() {
    FragColor = hasDiffuseTexture ? texture(texture_diffuse1, vTexCoords) : vec4(material_diffuseColor, 1.0f);
}
