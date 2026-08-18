#version 450 core

in vec4 gFragPos;

uniform vec3 lightPos;
uniform float far;

void main()
{
    float lightDistance = length(gFragPos.xyz - lightPos);
    lightDistance = lightDistance / far;
    gl_FragDepth = lightDistance;
}  