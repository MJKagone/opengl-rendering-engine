#version 450 core

layout(location = 0) in vec2 cornerPosition; // quad corner, +/-2 (sigma units)
layout(location = 1) in int splatIndex;      // per-instance, sorted back-to-front

// Mirrors GPUSplat in include/GaussianCloud.hpp (std140, 272 bytes/splat)
struct Splat {
    vec4 posOpacity;  // xyz: position (auto-normalized space), w: opacity
    vec4 quat;        // quaternion (x, y, z, w), normalized
    vec4 scaleRough;  // xyz: scale, w: roughness
    vec4 colorMetal; // rgb: base color (base_color or f_dc), a: metallic
    vec4 normal;      // xyz: unit normal
    vec4 sh[12];      // f_rest_0..44 (channel-major ply order), f_dc in sh[11].yzw
};
layout(std140, binding = 0) readonly buffer SplatBuffer {
    Splat splats[];
};

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 viewport;
uniform vec3 viewPos;
uniform int shDegree;
uniform mat4 dirLightSpaceMatrix;

// Raw appearance (SH)
out vec3 vSHColor;
// PBR appearance
out vec3 vColor;
out vec3 vNormal;
out float vRoughness;
out float vMetallic;
out float vOpacity;
out vec3 vFragPos;
out vec4 vFragPosLightSpace;
// Gaussian falloff
out vec2 vCenter; // splat center in pixel coords
out vec3 vConic;  // inverse 2D covariance: xx, xy, yy

const float SH_C0 = 0.28209479177387814;
const float SH_C1 = 0.4886025119029199;
const float SH_C2_0 = 1.0925484305920792;
const float SH_C2_1 = -1.0925484305920792;
const float SH_C2_2 = 0.31539156425292014;
const float SH_C2_3 = -1.0925484305920792;
const float SH_C2_4 = 0.5462742152960396;
const float SH_C3_0 = -0.5900435899266435;
const float SH_C3_1 = 2.890611442640554;
const float SH_C3_2 = -0.4570457994644658;
const float SH_C3_3 = 0.3731763325901154;
const float SH_C3_4 = -0.4570457994644658;
const float SH_C3_5 = 1.445305721320277;
const float SH_C3_6 = -0.5900435899266435;

// f_rest coefficient k (1..15) of channel ch (0..2). The ply stores f_rest
// channel-major (INRIA layout): f_rest_{c*15 + (k-1)}, 15 coefficients per
// channel at max degree 3
float shF(int k, int ch) {
    int i = ch * 15 + (k - 1);
    vec4 v = splats[splatIndex].sh[i / 4];
    return v[i % 4];
}
vec3 shC(int k) {
    return vec3(shF(k, 0), shF(k, 1), shF(k, 2));
}

mat3 quatToMat3(vec4 q) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    return mat3(
        1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y + w * z), 2.0 * (x * z - w * y),
        2.0 * (x * y - w * z), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z + w * x),
        2.0 * (x * z + w * y), 2.0 * (y * z - w * x), 1.0 - 2.0 * (x * x + y * y)
    );
}

void offscreen() {
    gl_Position = vec4(0.0, 0.0, 2.0, 1.0); // outside the clip volume
}

void main() {
    Splat s = splats[splatIndex];

    vec4 worldPos4 = model * vec4(s.posOpacity.xyz, 1.0);
    vec3 worldPos = worldPos4.xyz;
    vec4 camPos4 = view * worldPos4;
    vec4 clipPos = projection * camPos4;

    // Cull splats behind (or practically at) the camera plane
    if (clipPos.w <= 0.0 || camPos4.z > -0.1) {
        offscreen();
        return;
    }

    vec2 ndcCenter = clipPos.xy / clipPos.w;

    // --- Spherical harmonics: raw trained appearance ---
    // The SH is baked in the ply's own frame, so evaluate it with the view
    // direction rotated back into that frame (entity rotations included).
    vec3 dirWorld = normalize(worldPos - viewPos);
    vec3 dir = normalize(transpose(mat3(model)) * dirWorld);
    vec3 fdc = vec3(s.sh[11].y, s.sh[11].z, s.sh[11].w);
    vec3 sh = vec3(0.5) + SH_C0 * fdc;
    if (shDegree > 0) {
        sh += -SH_C1 * dir.y * shC(1)
            + SH_C1 * dir.z * shC(2)
            - SH_C1 * dir.x * shC(3);
    }
    if (shDegree > 1) {
        float xx = dir.x * dir.x, yy = dir.y * dir.y, zz = dir.z * dir.z;
        sh += SH_C2_0 * dir.x * dir.y * shC(4)
            + SH_C2_1 * dir.y * dir.z * shC(5)
            + SH_C2_2 * (2.0 * zz - xx - yy) * shC(6)
            + SH_C2_3 * dir.x * dir.z * shC(7)
            + SH_C2_4 * (xx - yy) * shC(8);
    }
    if (shDegree > 2) {
        float xx = dir.x * dir.x, yy = dir.y * dir.y, zz = dir.z * dir.z;
        sh += SH_C3_0 * dir.y * (3.0 * xx - yy) * shC(9)
            + SH_C3_1 * dir.x * dir.y * dir.z * shC(10)
            + SH_C3_2 * dir.y * (5.0 * zz - 1.0) * shC(11)
            + SH_C3_3 * dir.z * (5.0 * zz - 3.0) * shC(12)
            + SH_C3_4 * dir.x * (5.0 * zz - 1.0) * shC(13)
            + SH_C3_5 * dir.z * (xx - yy) * shC(14)
            + SH_C3_6 * dir.x * (xx - 3.0 * yy) * shC(15);
    }
    // SH appearance is baked display-space color; convert to linear so the
    // shared tone mapping/gamma pipeline reproduces it
    vSHColor = pow(clamp(sh, 0.0, 1.0), vec3(2.2));

    // --- PBR inputs ---
    vColor = s.colorMetal.rgb;
    vNormal = normalize(transpose(inverse(mat3(model))) * s.normal.xyz);
    vRoughness = s.scaleRough.w;
    vMetallic = s.colorMetal.a;
    vOpacity = s.posOpacity.w;
    vFragPos = worldPos;
    vFragPosLightSpace = dirLightSpaceMatrix * worldPos4;

    // --- 2D covariance projection (EWA splatting) ---
    vec2 focal = vec2(projection[0][0], projection[1][1]) * viewport * 0.5;
    mat3 R = quatToMat3(s.quat);
    mat3 S = mat3(
        s.scaleRough.x, 0.0, 0.0,
        0.0, s.scaleRough.y, 0.0,
        0.0, 0.0, s.scaleRough.z
    );
    mat3 T = mat3(view) * mat3(model) * R * S; // splat-local -> camera space
    mat3 covCam = T * transpose(T);            // camera-space covariance

    // Perspective projection Jacobian rows at the splat's camera position
    float invZ = 1.0 / camPos4.z;
    float invZ2 = invZ * invZ;
    vec3 jrowX = vec3(focal.x * invZ, 0.0, -focal.x * camPos4.x * invZ2);
    vec3 jrowY = vec3(0.0, focal.y * invZ, -focal.y * camPos4.y * invZ2);

    float a = dot(jrowX, covCam * jrowX);
    float b = dot(jrowX, covCam * jrowY);
    float d = dot(jrowY, covCam * jrowY);

    // Low-pass dilation to keep sub-pixel splats stable
    a += 0.3;
    d += 0.3;

    // Eigen-decomposition of the 2x2 covariance
    float mid = 0.5 * (a + d);
    float rad = sqrt(max(((a - d) * 0.5) * ((a - d) * 0.5) + b * b, 0.0));
    float l1 = mid + rad; // major eigenvalue
    float l2 = mid - rad; // minor eigenvalue
    if (l2 <= 0.0) {
        offscreen();
        return;
    }
    // Eigenvector of the major eigenvalue (guard against the b == 0,
    // a == d case where tmp vanishes; any orientation is valid there)
    vec2 tmp = vec2(b, l1 - a);
    if (dot(tmp, tmp) < 1e-20) tmp = vec2(1.0, 0.0);
    vec2 e1 = normalize(tmp);
    vec2 e2 = vec2(-e1.y, e1.x); // minor eigenvector (perpendicular)
    // Cap giant splats near the camera; scale the minor axis proportionally
    float r1 = min(sqrt(l1), 1024.0);
    float r2 = r1 * sqrt(l2 / l1);
    vec2 majorAxis = e1 * r1;
    vec2 minorAxis = e2 * r2;

    // Inverse covariance (conic) for the fragment falloff
    float det = a * d - b * b;
    if (det < 1e-12) {
        offscreen();
        return;
    }
    vConic = vec3(d, -b, a) / det;
    vCenter = (ndcCenter * 0.5 + 0.5) * viewport;

    // Expand the quad along the eigen-axes (pixel offset -> NDC)
    vec2 deltaScreen = cornerPosition.x * majorAxis + cornerPosition.y * minorAxis;
    vec2 deltaNDC = deltaScreen * 2.0 / viewport;
    gl_Position = vec4(ndcCenter + deltaNDC, clipPos.z / clipPos.w, 1.0);
}
