#version 450 core

in vec2 vTexCoords;

out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float exposure;

void main() {
    vec3 hdrColor = texture(screenTexture, vTexCoords).rgb;
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    FragColor = vec4(mapped, 1.0);
}