#version 450 core

// Raw 3DGS appearance: view-dependent SH color evaluated per splat in the
// vertex shader, composited back-to-front with the gaussian alpha falloff.

in vec3 vSHColor;
in float vOpacity;
in vec2 vCenter;
in vec3 vConic;

out vec4 FragColor;

void main() {
    vec2 d = gl_FragCoord.xy - vCenter;
    float power = -0.5 * (vConic.x * d.x * d.x + vConic.z * d.y * d.y) - vConic.y * d.x * d.y;
    if (power > 0.0) discard; // numerical safety outside the covariance

    float alpha = min(0.99, vOpacity * exp(power));
    if (alpha < 1.0 / 255.0) discard;

    FragColor = vec4(vSHColor, alpha);
}
