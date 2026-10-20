#version 450 core

layout (location = 0) in vec3 aPos;

out vec3 vTexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 rotation = mat4(1.0);

void main()
{
    // Rotate the sample direction so the environment appears rotated;
    // cube geometry itself stays aligned with the view.
    vTexCoords = (rotation * vec4(aPos, 0.0)).xyz;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
