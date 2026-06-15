#version 460 core
out vec4 FragColor;

// The 3D position of the skybox cube's vertices
in vec3 TexCoords; 

uniform sampler2D skybox;

// 1.0 / (2.0 * PI) and 1.0 / PI
const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 sampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = sampleSphericalMap(normalize(TexCoords)); // Convert 3D vector to 2D UV
    vec3 color = texture(skybox, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}