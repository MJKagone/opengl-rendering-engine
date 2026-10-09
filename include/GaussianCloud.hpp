#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// One 3D Gaussian splat, packed into vec4s so that the C++ layout matches the
// std140 'Splat' struct in shaders/vertex/vs_splat.glsl exactly (272 bytes).
struct GPUSplat {
    glm::vec4 posOpacity;  // xyz: position (auto-normalized space), w: sigmoid(opacity)
    glm::vec4 quat;        // quaternion, stored (x, y, z, w), normalized on load
    glm::vec4 scaleRough;  // xyz: exp(scale_i), w: roughness
    glm::vec4 colorMetal; // rgb: base color (base_color if present, else f_dc), a: metallic
    glm::vec4 normal;      // xyz: unit normal (from file, or gaussian's shortest axis)
    glm::vec4 sh[12];      // f_rest_0..44 in ply order (i = 3*coef + channel),
                           // followed by f_dc_0..2 stored in sh[11].yzw
};

// A cloud of 3D Gaussians loaded from a .ply file. Supports the standard
// trained 3DGS ply layout (INRIA: x/y/z, rot, scale, opacity, f_dc, f_rest)
// and the Relightable 3DGS (R3DG) layout, which additionally stores nx/ny/nz,
// base_color, roughness and baked light transport (incidents_/visibility_
// properties are parsed but ignored; the PBR splat shader lights splats with
// the scene lights instead).
//
// Clouds are auto-normalized on load: recentered on the origin and uniformly
// rescaled so the bounding box diagonal is ~targetDiagonal, so they fit the
// engine's default camera/frustum. Entity transforms compose on top.
//
// Rendering: one instanced 6-vertex quad per splat. Splat data lives in an
// SSBO (binding 0); the per-instance attribute is the splat index. Every
// frame the CPU sorts splats back-to-front (radix sort on view depth) and
// re-uploads the sorted splat indices (updateSort + draw). The sort is
// skipped when the view and entity transform are unchanged from the last
// sort, so a static camera pays nothing per frame.
class GaussianCloud {
public:
    size_t count = 0;
    int shDegree = 0;             // 0..3, from the number of f_rest properties
    bool hasNormals = false;      // real nx/ny/nz properties present in the file
    bool hasBaseColor = false;    // R3DG-style base_color present (PBR albedo)
    bool hasRoughness = false;    // R3DG-style roughness property present
    glm::vec3 centroid = glm::vec3(0.0f); // post-normalization, for entity ordering

    // minOpacity: optional visibility cull threshold in opacity units. If > 0,
    // splats with decoded opacity below it are dropped at load time (they can
    // never produce a visible fragment). 0 disables culling entirely.
    bool loadFromPLY(const std::string& path, float targetDiagonal = 30.0f, float minOpacity = 0.0f);
    void updateSort(const glm::mat4& view, const glm::mat4& model);
    void draw() const;

    GaussianCloud() = default;
    ~GaussianCloud();
    GaussianCloud(const GaussianCloud&) = delete;
    GaussianCloud& operator=(const GaussianCloud&) = delete;

private:
    std::vector<GPUSplat> splats;
    std::vector<int> sortedIndices;
    std::vector<uint64_t> radixKeysA, radixKeysB; // packed (depth key << 32 | index)
    std::vector<uint32_t> radixHistogram;
    GLuint ssbo = 0, vao = 0, quadVBO = 0, indexVBO = 0;
    glm::mat4 lastView = glm::mat4(1.0f);
    glm::mat4 lastModel = glm::mat4(1.0f);
    bool hasSortState = false; // true only if the index buffer matches (view, model)

    void upload();
};

// --- PLY parsing internals -------------------------------------------------

enum PlyTypeKind { PLY_FLOAT, PLY_SINT, PLY_UINT };

struct PlyProperty {
    std::string name;
    int size = 0;
    PlyTypeKind kind = PLY_FLOAT;
    size_t offset = 0; // byte offset within the element row (binary)
    size_t index = 0;  // position within the element's property list (ascii)
};

struct PlyElement {
    std::string name;
    size_t count = 0;
    std::vector<PlyProperty> props;
    size_t stride = 0;
};

inline bool plyTypeInfo(const std::string& t, int& size, PlyTypeKind& kind) {
    if (t == "float" || t == "float32") { size = 4; kind = PLY_FLOAT; return true; }
    if (t == "double" || t == "float64") { size = 8; kind = PLY_FLOAT; return true; }
    if (t == "char" || t == "int8") { size = 1; kind = PLY_SINT; return true; }
    if (t == "uchar" || t == "uint8") { size = 1; kind = PLY_UINT; return true; }
    if (t == "short" || t == "int16") { size = 2; kind = PLY_SINT; return true; }
    if (t == "ushort" || t == "uint16") { size = 2; kind = PLY_UINT; return true; }
    if (t == "int" || t == "int32") { size = 4; kind = PLY_SINT; return true; }
    if (t == "uint" || t == "uint32") { size = 4; kind = PLY_UINT; return true; }
    return false;
}

inline double readBinaryScalar(const unsigned char* p, const PlyProperty& prop) {
    switch (prop.kind) {
        case PLY_FLOAT: {
            if (prop.size == 4) { float v; std::memcpy(&v, p, 4); return (double)v; }
            double v; std::memcpy(&v, p, 8); return v;
        }
        case PLY_SINT: {
            if (prop.size == 1) { int8_t v; std::memcpy(&v, p, 1); return (double)v; }
            if (prop.size == 2) { int16_t v; std::memcpy(&v, p, 2); return (double)v; }
            int32_t v; std::memcpy(&v, p, 4); return (double)v;
        }
        case PLY_UINT: {
            if (prop.size == 1) { uint8_t v; std::memcpy(&v, p, 1); return (double)v; }
            if (prop.size == 2) { uint16_t v; std::memcpy(&v, p, 2); return (double)v; }
            uint32_t v; std::memcpy(&v, p, 4); return (double)v;
        }
    }
    return 0.0;
}

inline float sigmoidf(float x) { return 1.0f / (1.0f + std::exp(-x)); }

// --- GaussianCloud implementation ------------------------------------------

inline bool GaussianCloud::loadFromPLY(const std::string& path, float targetDiagonal, float minOpacity) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open splat file: " << path << std::endl;
        return false;
    }

    // --- Parse the text header ---
    std::string line;
    std::getline(file, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "ply") {
        std::cerr << "Not a PLY file: " << path << std::endl;
        return false;
    }

    std::string format;
    std::vector<PlyElement> elements;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ss(line);
        std::string keyword;
        ss >> keyword;
        if (keyword == "comment" || keyword == "obj_info") {
            continue;
        } else if (keyword == "format") {
            ss >> format;
        } else if (keyword == "element") {
            PlyElement el;
            ss >> el.name >> el.count;
            elements.push_back(el);
        } else if (keyword == "property") {
            if (elements.empty()) {
                std::cerr << "PLY property outside of element in: " << path << std::endl;
                return false;
            }
            std::string type, name;
            ss >> type >> name;
            if (type == "list") {
                std::cerr << "PLY list properties are not supported: " << path << std::endl;
                return false;
            }
            PlyProperty prop;
            prop.name = name;
            if (!plyTypeInfo(type, prop.size, prop.kind)) {
                std::cerr << "Unsupported PLY property type '" << type << "' in: " << path << std::endl;
                return false;
            }
            PlyElement& el = elements.back();
            prop.offset = el.stride;
            prop.index = el.props.size();
            el.stride += (size_t)prop.size;
            el.props.push_back(prop);
        } else if (keyword == "end_header") {
            break;
        }
    }

    // Find the vertex element (binary data of preceding elements is skipped)
    size_t vertexDataOffset = 0;
    const PlyElement* vertexEl = nullptr;
    for (const auto& el : elements) {
        if (el.name == "vertex") { vertexEl = &el; break; }
        vertexDataOffset += el.stride * el.count;
    }
    if (!vertexEl) {
        std::cerr << "PLY file has no vertex element: " << path << std::endl;
        return false;
    }
    if (format != "binary_little_endian" && format != "ascii") {
        std::cerr << "Unsupported PLY format '" << format << "' (need binary_little_endian or ascii): " << path << std::endl;
        return false;
    }

    // --- Locate the properties we care about ---
    // Fixed slots for interesting values; unknown properties (f_rest beyond
    // degree 3, incidents_*, visibility_*, ...) are skipped via offsets.
    const int SLOT_X = 0, SLOT_Y = 1, SLOT_Z = 2;
    const int SLOT_ROT0 = 3, SLOT_ROT1 = 4, SLOT_ROT2 = 5, SLOT_ROT3 = 6;
    const int SLOT_SCALE0 = 7, SLOT_SCALE1 = 8, SLOT_SCALE2 = 9;
    const int SLOT_OPACITY = 10;
    const int SLOT_FDC0 = 11, SLOT_FDC1 = 12, SLOT_FDC2 = 13;
    const int SLOT_FREST0 = 14; // 45 slots: i = 3*coef + channel
    const int SLOT_NX = 59, SLOT_NY = 60, SLOT_NZ = 61;
    const int SLOT_BC0 = 62, SLOT_BC1 = 63, SLOT_BC2 = 64;
    const int SLOT_ROUGH = 65, SLOT_METAL = 66;
    const int SLOT_COUNT = 67;

    std::unordered_map<std::string, int> slotByName = {
        {"x", SLOT_X}, {"y", SLOT_Y}, {"z", SLOT_Z},
        {"rot_0", SLOT_ROT0}, {"rot_1", SLOT_ROT1}, {"rot_2", SLOT_ROT2}, {"rot_3", SLOT_ROT3},
        {"scale_0", SLOT_SCALE0}, {"scale_1", SLOT_SCALE1}, {"scale_2", SLOT_SCALE2},
        {"opacity", SLOT_OPACITY},
        {"f_dc_0", SLOT_FDC0}, {"f_dc_1", SLOT_FDC1}, {"f_dc_2", SLOT_FDC2},
        {"nx", SLOT_NX}, {"ny", SLOT_NY}, {"nz", SLOT_NZ},
        {"base_color_0", SLOT_BC0}, {"base_color_1", SLOT_BC1}, {"base_color_2", SLOT_BC2},
        {"roughness", SLOT_ROUGH}, {"metallic", SLOT_METAL},
    };
    for (int i = 0; i < 45; i++) {
        slotByName["f_rest_" + std::to_string(i)] = SLOT_FREST0 + i;
    }

    // Per-property slot lookup (-1 = ignored)
    const PlyProperty* slotProps[SLOT_COUNT] = {};
    std::vector<int> slotByPropIndex(vertexEl->props.size(), -1);
    for (const auto& prop : vertexEl->props) {
        auto it = slotByName.find(prop.name);
        if (it == slotByName.end()) continue;
        slotProps[it->second] = &prop;
        slotByPropIndex[prop.index] = it->second;
    }

    // Required 3DGS properties
    const char* required[] = {"x", "y", "z", "opacity", "scale_0", "scale_1", "scale_2",
                              "rot_0", "rot_1", "rot_2", "rot_3", "f_dc_0", "f_dc_1", "f_dc_2"};
    for (const char* name : required) {
        if (!slotProps[slotByName[name]]) {
            std::cerr << "PLY file '" << path << "' is missing required 3DGS property '" << name << "'" << std::endl;
            return false;
        }
    }

    // SH degree from the number of f_rest properties (0/9/24/45 -> 0/1/2/3)
    int nRest = 0;
    while (nRest < 45 && slotProps[SLOT_FREST0 + nRest]) nRest++;
    shDegree = (nRest > 0) ? (int)std::lround(std::sqrt(nRest / 3.0 + 1.0)) - 1 : 0;
    shDegree = std::max(0, std::min(3, shDegree));
    hasNormals = slotProps[SLOT_NX] && slotProps[SLOT_NY] && slotProps[SLOT_NZ];
    hasBaseColor = slotProps[SLOT_BC0] && slotProps[SLOT_BC1] && slotProps[SLOT_BC2];
    hasRoughness = slotProps[SLOT_ROUGH] != nullptr;

    // --- Read all rows ---
    const size_t n = vertexEl->count;
    const size_t stride = vertexEl->stride;
    std::vector<unsigned char> data;

    if (format == "binary_little_endian") {
        file.seekg((std::streamsize)vertexDataOffset, std::ios::cur);
        data.resize(stride * n);
        file.read((char*)data.data(), (std::streamsize)data.size());
        if ((size_t)file.gcount() != data.size()) {
            std::cerr << "Truncated PLY data in: " << path << std::endl;
            return false;
        }
    }

    std::istringstream asciiStream;
    if (format == "ascii") {
        std::string rest((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        asciiStream.str(rest);
    }

    splats.resize(n);
    double vals[SLOT_COUNT];
    size_t kept = 0;   // splats passing the visibility cull (if enabled)
    size_t culled = 0;

    for (size_t i = 0; i < n; i++) {
        // Gather interesting values for this row
        if (format == "binary_little_endian") {
            const unsigned char* row = &data[i * stride];
            for (int s = 0; s < SLOT_COUNT; s++) {
                if (slotProps[s]) vals[s] = readBinaryScalar(row + slotProps[s]->offset, *slotProps[s]);
                else vals[s] = 0.0;
            }
        } else {
            for (const auto& prop : vertexEl->props) {
                double v = 0.0;
                if (!(asciiStream >> v)) {
                    std::cerr << "Truncated PLY data in: " << path << std::endl;
                    return false;
                }
                int slot = slotByPropIndex[prop.index];
                if (slot >= 0) vals[slot] = v;
            }
        }

        GPUSplat& g = splats[kept];

        // Position + activated opacity (logit -> sigmoid)
        g.posOpacity = glm::vec4((float)vals[SLOT_X], (float)vals[SLOT_Y], (float)vals[SLOT_Z],
                                 sigmoidf((float)vals[SLOT_OPACITY]));

        // Quaternion: rot_0 is the real part (w); stored quats are NOT
        // normalized in practice, so normalize here
        glm::quat q((float)vals[SLOT_ROT0], (float)vals[SLOT_ROT1], (float)vals[SLOT_ROT2], (float)vals[SLOT_ROT3]);
        float ql = glm::length(q);
        q = (ql > 1e-8f) ? q / ql : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        g.quat = glm::vec4(q.x, q.y, q.z, q.w);

        // Log scales -> linear
        glm::vec3 scale(std::exp((float)vals[SLOT_SCALE0]),
                        std::exp((float)vals[SLOT_SCALE1]),
                        std::exp((float)vals[SLOT_SCALE2]));
        g.scaleRough = glm::vec4(scale, hasRoughness ? sigmoidf((float)vals[SLOT_ROUGH]) : 0.5f);

        // Base color: R3DG base_color (raw, needs sigmoid) or the SH DC term
        glm::vec3 baseColor;
        if (hasBaseColor) {
            baseColor = glm::vec3(sigmoidf((float)vals[SLOT_BC0]),
                                  sigmoidf((float)vals[SLOT_BC1]),
                                  sigmoidf((float)vals[SLOT_BC2]));
        } else {
            const float SH_C0 = 0.28209479177387814f;
            baseColor = glm::vec3(0.5f) + SH_C0 * glm::vec3((float)vals[SLOT_FDC0], (float)vals[SLOT_FDC1], (float)vals[SLOT_FDC2]);
        }
        g.colorMetal = glm::vec4(baseColor, (float)vals[SLOT_METAL]); // metallic optional, 0 if absent

        // Normal: from the file (R3DG stores unnormalized) or derived from
        // the gaussian's shortest axis
        glm::vec3 normal(0.0f);
        if (hasNormals) {
            normal = glm::vec3((float)vals[SLOT_NX], (float)vals[SLOT_NY], (float)vals[SLOT_NZ]);
        }
        if (glm::length(normal) < 1e-6f) {
            glm::mat3 R = glm::mat3_cast(q);
            int minAxis = (scale.x <= scale.y && scale.x <= scale.z) ? 0 : ((scale.y <= scale.z) ? 1 : 2);
            normal = R[minAxis]; // column of the rotation matrix = scaled axis direction
        }
        g.normal = glm::vec4(glm::normalize(normal), 0.0f);

        // SH coefficients: f_rest in ply order (i = 3*coef + channel),
        // f_dc parked in the 3 spare slots at the end of the sh array
        for (int r = 0; r < 45; r++) {
            g.sh[r / 4][r % 4] = (r < nRest) ? (float)vals[SLOT_FREST0 + r] : 0.0f;
        }
        g.sh[11].y = (float)vals[SLOT_FDC0];
        g.sh[11].z = (float)vals[SLOT_FDC1];
        g.sh[11].w = (float)vals[SLOT_FDC2];

        // Optional load-time cull requested via --culling: splats with
        // opacity below the threshold can never produce a visible fragment
        // past the fragment shader's alpha discard
        if (minOpacity > 0.0f && g.posOpacity.w < minOpacity) {
            culled++;
        } else {
            kept++;
        }
    }
    splats.resize(kept);
    count = kept;
    if (culled > 0) {
        std::cout << "  culled " << culled << " splats (opacity < "
                  << (int)std::lround(minOpacity * 255.0f) << "/255)" << std::endl;
    }

    // --- Auto-normalize: recenter on the origin, rescale to targetDiagonal ---
    glm::vec3 bbmin(FLT_MAX), bbmax(-FLT_MAX);
    for (const auto& g : splats) {
        bbmin = glm::min(bbmin, glm::vec3(g.posOpacity));
        bbmax = glm::max(bbmax, glm::vec3(g.posOpacity));
    }
    glm::vec3 center = (bbmin + bbmax) * 0.5f;
    float diag = glm::length(bbmax - bbmin);
    float factor = (diag > 1e-9f) ? targetDiagonal / diag : 1.0f;
    centroid = glm::vec3(0.0f);
    for (auto& g : splats) {
        glm::vec3 p = (glm::vec3(g.posOpacity) - center) * factor;
        g.posOpacity.x = p.x; g.posOpacity.y = p.y; g.posOpacity.z = p.z;
        g.scaleRough.x *= factor; g.scaleRough.y *= factor; g.scaleRough.z *= factor;
        centroid += p;
    }
    if (count > 0) centroid /= (float)count;

    upload();

    std::cout << "  " << count << " gaussians, SH degree " << shDegree
              << (hasBaseColor ? ", relightable (normals + base color + roughness)"
                               : (hasNormals ? ", normals" : ""))
              << std::endl;
    return true;
}

inline void GaussianCloud::upload() {
    if (count == 0) return;

    // Splat data SSBO (binding 0, read by vs_splat.glsl)
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(count * sizeof(GPUSplat)), splats.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Quad corners (+/-2 sigma extents), shared by all splats
    float quadVertices[] = {
        -2.0f, -2.0f,
         2.0f, -2.0f,
         2.0f,  2.0f,
        -2.0f, -2.0f,
         2.0f,  2.0f,
        -2.0f,  2.0f,
    };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &indexVBO);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Per-instance sorted splat index, orphaned and refilled by updateSort
    glBindBuffer(GL_ARRAY_BUFFER, indexVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(int)), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_INT, sizeof(int), (void*)0);
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    sortedIndices.resize(count);
    radixKeysA.resize(count);
    radixKeysB.resize(count);
    radixHistogram.assign(65536, 0);
}

// Order-preserving mapping of a float to its bit pattern as uint32
// (ascending u <=> ascending float for all finite values), so the raw key can
// be radix-sorted to get the float order
inline uint32_t floatToOrderedBits(float v) {
    uint32_t u;
    std::memcpy(&u, &v, 4);
    return (u & 0x80000000u) ? ~u : (u ^ 0x80000000u);
}

inline void GaussianCloud::updateSort(const glm::mat4& view, const glm::mat4& model) {
    if (count == 0) return;

    // Matrices are bit-identical between frames when neither the camera nor
    // the entity transform has changed: reuse the existing index buffer
    if (hasSortState && lastView == view && lastModel == model) return;
    lastView = view;
    lastModel = model;
    hasSortState = true;

    // View-space depth of each splat (worldPos = model * pos). Depth is
    // negative in front of the camera, so ascending order = farthest first.
    glm::mat4 vm = view * model;
    float zx = vm[0].z, zy = vm[1].z, zz = vm[2].z, zw = vm[3].z;
    uint64_t* keys = radixKeysA.data();
    for (size_t i = 0; i < count; i++) {
        const glm::vec4& p = splats[i].posOpacity;
        float depth = p.x * zx + p.y * zy + p.z * zz + zw;
        keys[i] = ((uint64_t)floatToOrderedBits(depth) << 32) | (uint32_t)i;
    }

    // Four 16-digit LSD radix passes over the packed 64-bit keys; the low
    // 32 bits (the splat index, used as a tiebreaker) come along for free
    uint64_t* src = radixKeysA.data();
    uint64_t* dst = radixKeysB.data();
    for (int pass = 0; pass < 4; pass++) {
        int shift = pass * 16;
        std::fill(radixHistogram.begin(), radixHistogram.end(), 0u);
        for (size_t i = 0; i < count; i++) {
            radixHistogram[(src[i] >> shift) & 0xFFFFu]++;
        }
        uint32_t running = 0;
        for (int b = 0; b < 65536; b++) {
            uint32_t c = radixHistogram[b];
            radixHistogram[b] = running;
            running += c;
        }
        for (size_t i = 0; i < count; i++) {
            dst[radixHistogram[(src[i] >> shift) & 0xFFFFu]++] = src[i];
        }
        std::swap(src, dst);
    }
    // Four passes: the sorted keys ended up back in radixKeysA
    for (size_t i = 0; i < count; i++) {
        sortedIndices[i] = (int)(uint32_t)(radixKeysA[i] & 0xFFFFFFFFu);
    }

    // Orphan and refill the instance index buffer
    glBindBuffer(GL_ARRAY_BUFFER, indexVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(int)), sortedIndices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

inline void GaussianCloud::draw() const {
    if (count == 0) return;
    glBindVertexArray(vao);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)count);
}

inline GaussianCloud::~GaussianCloud() {
    if (ssbo) glDeleteBuffers(1, &ssbo);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    if (indexVBO) glDeleteBuffers(1, &indexVBO);
    if (vao) glDeleteVertexArrays(1, &vao);
}
