#pragma once

#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.hpp"
#include "Shader.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;

GLuint TextureFromFile(const char *path, const string &directory, bool gamma);
GLuint TextureFromEmbedded(const aiTexture *embeddedTex, bool gamma);

class Model 
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh> meshes;
    vector<Mesh> opaqueMeshes;
    vector<Mesh> transparentMeshes;
    string directory;
    bool gammaCorrection;
    bool hasEmbeddedTextures;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = true) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void draw(Shader &shader)
    {
        for(size_t i = 0; i < meshes.size(); i++)
            meshes[i].draw(shader);
    }

    void drawOpaque(Shader &shader)
    {
        for(size_t i = 0; i < opaqueMeshes.size(); i++)
            opaqueMeshes[i].draw(shader);
    }

    void drawTransparent(Shader &shader)
    {
        for(size_t i = 0; i < transparentMeshes.size(); i++)
            transparentMeshes[i].draw(shader);
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // Check format from extension
        string lowerPath = path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
        hasEmbeddedTextures = (lowerPath.rfind(".glb") != string::npos || lowerPath.rfind(".gltf") != string::npos);

        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            Mesh processedMesh = processMesh(mesh, scene);
            if (processedMesh.isTransparent)
                transparentMeshes.push_back(processedMesh);
            else
                opaqueMeshes.push_back(processedMesh);
            meshes.push_back(processedMesh);
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<GLuint> indices;
        vector<Texture> textures;

        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", this->gammaCorrection, scene);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        bool hasDiffuseTexture = !diffuseMaps.empty();
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", false, scene);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        bool hasSpecularTexture = !specularMaps.empty();
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", false, scene);
        // Fallback to HEIGHT if NORMALS is empty (common for .obj files)
        if (normalMaps.empty()) {
            normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", false, scene);
        }
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        bool hasNormalTexture = !normalMaps.empty();
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height", false, scene);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        // 5. emission maps
        std::vector<Texture> emissionMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emission", this->gammaCorrection, scene);
        textures.insert(textures.end(), emissionMaps.begin(), emissionMaps.end());
        bool hasEmissionTexture = !emissionMaps.empty();
        // 6. metallic maps
        std::vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic", false, scene);
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
        bool hasMetallicTexture = !metallicMaps.empty();
        // 7. roughness maps
        std::vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness", false, scene);
        // FBX fallback: roughness packed into shininess
        if (roughnessMaps.empty()) {
            roughnessMaps = loadMaterialTextures(material, aiTextureType_SHININESS, "texture_roughness", false, scene);
        }
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
        bool hasRoughnessTexture = !roughnessMaps.empty();
        // 8. ambient occlusion maps
        std::vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION, "texture_ao", false, scene);
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
        bool hasAOTexture = !aoMaps.empty();

        // Robust material color extraction chain
        glm::vec3 diffuseColor(1.0f); 
        aiColor4D pbrColor(1.0f, 1.0f, 1.0f, 1.0f);
        aiColor3D legacyColor(1.0f, 1.0f, 1.0f);

        // Try modern PBR base color slot first
        if (material->Get(AI_MATKEY_BASE_COLOR, pbrColor) == AI_SUCCESS) {
            diffuseColor = glm::vec3(pbrColor.r, pbrColor.g, pbrColor.b);
        }
        // Fall back to legacy diffuse color slot
        else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, legacyColor) == AI_SUCCESS) {
            diffuseColor = glm::vec3(legacyColor.r, legacyColor.g, legacyColor.b);
        }

        // If no diffuse texture exists and the color is still pure black, check the ambient color slot
        if (diffuseMaps.empty() && diffuseColor == glm::vec3(0.0f)) {
            if (material->Get(AI_MATKEY_COLOR_AMBIENT, legacyColor) == AI_SUCCESS) {
                diffuseColor = glm::vec3(legacyColor.r, legacyColor.g, legacyColor.b);
            }
            // Safeguard: if still black, use dark gray so geometry isn't completely unlit
            if (diffuseColor == glm::vec3(0.0f)) {
                diffuseColor = glm::vec3(0.15f);
            }
        }

        float shininess = 32.0f;
        float extractedShininess;

        // Try to get the shininess from the material
        if (material->Get(AI_MATKEY_SHININESS, extractedShininess) == AI_SUCCESS) {
            if (extractedShininess > 1.0f) {
                shininess = extractedShininess * 2.2f;
            }
        }

        float opacity = 1.0f;
        // Try to get the opacity from the material
        if (material->Get(AI_MATKEY_OPACITY, opacity) != AI_SUCCESS) {
            opacity = 1.0f; // Default to fully opaque if not specified
        }

        bool isTransparent = (opacity < 0.99f);
        
        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures, diffuseColor, hasDiffuseTexture, hasSpecularTexture, hasNormalTexture, hasMetallicTexture, hasRoughnessTexture, hasAOTexture, hasEmissionTexture, hasEmbeddedTextures, shininess, opacity, isTransparent);
        
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName, bool gamma, const aiScene* scene)
    {
        vector<Texture> textures;
        for(int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(size_t j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    Texture cachedTexture = textures_loaded[j];
                    cachedTexture.type = typeName; 
                    textures.push_back(cachedTexture);
                    skip = true; 
                    break;
                }
            }
            if(!skip)
            {   
                GLuint textureID = 0;

                // Check for embedded textures (*0, *1)
                const aiTexture* embedded = (scene != nullptr) ? scene->GetEmbeddedTexture(str.C_Str()) : nullptr;
                if (embedded) {
                    textureID = TextureFromEmbedded(embedded, gamma);
                } else {
                    textureID = TextureFromFile(str.C_Str(), this->directory, gamma);
                }
                if (textureID != 0)
                {
                    Texture texture;
                    texture.id = textureID;
                    texture.type = typeName;
                    texture.path = str.C_Str();
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
                }
            }
        }
        return textures;
    }
};


// Helper: Normalize string (lowercase, replace spaces with underscores, strip common export suffixes)
std::string normalizeFilename(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    std::replace(name.begin(), name.end(), ' ', '_');
    
    // Strip common export suffixes
    const std::string suffix1 = "_(personalizado)";
    const std::string suffix2 = "_(personalizad";
    size_t pos;
    while ((pos = name.find(suffix1)) != std::string::npos) name.erase(pos, suffix1.length());
    while ((pos = name.find(suffix2)) != std::string::npos) name.erase(pos, suffix2.length());
    
    return name;
}

GLuint TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string rawPath = string(path);
    std::replace(rawPath.begin(), rawPath.end(), '\\', '/');

    string filename = rawPath;
    size_t lastSlash = filename.find_last_of('/');
    if (lastSlash != string::npos) {
        filename = filename.substr(lastSlash + 1);
    }

    string baseDir = directory;
    size_t dirLastSlash = baseDir.find_last_of('/');
    if (dirLastSlash != string::npos) {
        baseDir = baseDir.substr(0, dirLastSlash);
    }

    // 1. Standard candidate list
    std::vector<string> pathsToTry = {
        directory + '/' + rawPath,
        directory + '/' + filename,
        baseDir + "/textures/" + rawPath,
        baseDir + "/textures/" + filename,
        baseDir + '/' + rawPath,
        baseDir + '/' + filename
    };

    auto endsWith = [](const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() && 
               str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    // Extension permutations (.jpg <-> .jpeg, .png, etc.)
    std::vector<string> expandedPaths;
    for (const string& p : pathsToTry) {
        expandedPaths.push_back(p);
        if (endsWith(p, ".jpg"))  expandedPaths.push_back(p.substr(0, p.length() - 4) + ".jpeg");
        if (endsWith(p, ".jpeg")) expandedPaths.push_back(p.substr(0, p.length() - 5) + ".jpg");
        if (endsWith(p, ".png")) {
            expandedPaths.push_back(p.substr(0, p.length() - 4) + ".jpg");
            expandedPaths.push_back(p.substr(0, p.length() - 4) + ".jpeg");
        }
    }

    int width, height, nrComponents;
    unsigned char *data = nullptr;

    for (const string& p : expandedPaths) {
        data = stbi_load(p.c_str(), &width, &height, &nrComponents, 0);
        if (data) break;
    }

    // 2. Fuzzy directory scan fallback if exact matches fail
    if (!data) {
        string searchDir = baseDir + "/textures";
        if (fs::exists(searchDir) && fs::is_directory(searchDir)) {
            string normTarget = normalizeFilename(filename);
            string targetStem = normalizeFilename(fs::path(filename).stem().string());

            // Handle channel abbreviation mappings (_basecolor -> _b, etc.)
            string abbrevStem = targetStem;
            for (const auto& [full, abbr] : {
                std::pair{"_basecolor", "_b"}, {"_normal", "_n"},
                {"_roughness", "_r"}, {"_emissive", "_e"}, {"_metallic", "_m"}
            }) {
                size_t p = abbrevStem.rfind(full);
                if (p != std::string::npos) {
                    abbrevStem.replace(p, strlen(full), abbr);
                    break;
                }
            }

            for (const auto& entry : fs::directory_iterator(searchDir)) {
                if (!entry.is_regular_file()) continue;
                string entryName = normalizeFilename(entry.path().filename().string());
                string entryStem = normalizeFilename(entry.path().stem().string());

                // Match against normalized filename, stem, abbreviated stem, or prefix match
                if (entryName == normTarget || 
                    entryStem == targetStem || 
                    entryStem == abbrevStem ||
                    (targetStem.length() > 15 && entryStem.rfind(targetStem.substr(0, 15), 0) == 0)) 
                {
                    data = stbi_load(entry.path().string().c_str(), &width, &height, &nrComponents, 0);
                    if (data) break;
                }
            }
        }
    }

    if (data)
    {
        GLuint textureID;
        glGenTextures(1, &textureID);
        
        GLenum internalFormat = 0;
        GLenum dataFormat = 0;
        
        if (nrComponents == 1) {
            internalFormat = GL_RED;
            dataFormat = GL_RED;
        }
        else if (nrComponents == 2) {
            internalFormat = GL_RG;
            dataFormat = GL_RG;
        }
        else if (nrComponents == 3) {
            internalFormat = gamma ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4) {
            internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        if (nrComponents == 1) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        }
        
        // Tell OpenGL to expect 1-byte aligned data tightly packed by stbi_load
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return textureID;
    }
    else
    {
        std::cout << "Texture failed to load. Original path: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }
}

GLuint TextureFromEmbedded(const aiTexture *embeddedTex, bool gamma)
{
    int width, height, nrComponents;
    unsigned char *data = nullptr;

    if (embeddedTex->mHeight == 0) {
        // Compressed image buffer (PNG / JPEG inside .glb container)
        data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(embeddedTex->pcData),
            embeddedTex->mWidth, // mWidth holds buffer byte length when mHeight is 0
            &width, &height, &nrComponents, 0
        );
    } else {
        // Raw uncompressed texel array
        data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(embeddedTex->pcData),
            embeddedTex->mWidth * embeddedTex->mHeight * 4,
            &width, &height, &nrComponents, 0
        );
    }

    if (!data) {
        std::cout << "Failed to decode embedded texture from memory." << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    
    GLenum internalFormat = 0;
    GLenum dataFormat = 0;
    
    if (nrComponents == 1) {
        internalFormat = GL_RED;
        dataFormat = GL_RED;
    } else if (nrComponents == 2) {
        internalFormat = GL_RG;
        dataFormat = GL_RG;
    } else if (nrComponents == 3) {
        internalFormat = gamma ? GL_SRGB : GL_RGB;
        dataFormat = GL_RGB;
    } else if (nrComponents == 4) {
        internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
        dataFormat = GL_RGBA;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    if (nrComponents == 1) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}