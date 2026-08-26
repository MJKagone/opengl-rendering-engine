#pragma once

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.hpp"

#include <string>
#include <vector>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
	//bone indexes which will influence this vertex
	int m_BoneIDs[MAX_BONE_INFLUENCE];
	//weights from each bone
	float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
    GLuint id;
    string type;
    string path;
};

class Mesh {
public:
    // mesh Data
    GLuint VAO;
    vector<Vertex> vertices;
    vector<GLuint> indices;
    vector<Texture> textures;
    glm::vec3 diffuseColor;
    float shininess;
    float opacity;
    bool isTransparent;
    bool hasDiffuseTexture;
    bool hasSpecularTexture;
    bool hasNormalTexture;
    bool hasMetallicTexture;
    bool hasRoughnessTexture;
    bool hasAOTexture;
    bool hasEmissionTexture;

    // constructor
    Mesh(vector<Vertex> vertices, vector<GLuint> indices, vector<Texture> textures, glm::vec3 diffuseColor = glm::vec3(1.0f), bool hasDiffuseTexture = false, bool hasSpecularTexture = false, bool hasNormalTexture = false, bool hasMetallicTexture = false, bool hasRoughnessTexture = false, bool hasAOTexture = false, bool hasEmissionTexture = false, float shininess = 32.0f, float opacity = 1.0f, bool isTransparent = false)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->diffuseColor = diffuseColor;
        this->hasDiffuseTexture = hasDiffuseTexture;
        this->hasSpecularTexture = hasSpecularTexture;
        this->hasNormalTexture = hasNormalTexture;
        this->hasMetallicTexture = hasMetallicTexture;
        this->hasRoughnessTexture = hasRoughnessTexture;
        this->hasAOTexture = hasAOTexture;
        this->hasEmissionTexture = hasEmissionTexture;
        this->shininess = shininess;
        this->opacity = opacity;
        this->isTransparent = isTransparent;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    // render the mesh
    void draw(Shader &shader) 
    {
        // bind appropriate textures
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;
        unsigned int emissionNr = 1;
        unsigned int metallicNr = 1;
        unsigned int roughnessNr = 1;
        unsigned int aoNr = 1;

        for (int i = 0; i < static_cast<int>(textures.size()); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
            // retrieve texture number (the N in diffuse_textureN)
            string number;
            string name = textures[i].type;
            
            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++);
            else if(name == "texture_normal")
                number = std::to_string(normalNr++);
            else if(name == "texture_height")
                number = std::to_string(heightNr++);
            else if(name == "texture_emission") 
                number = std::to_string(emissionNr++);
            else if (name == "texture_metallic")
                number = std::to_string(metallicNr++);
            else if(name == "texture_roughness")
                number = std::to_string(roughnessNr++);
            else if(name == "texture_ao")
                number = std::to_string(aoNr++);

            // now set the sampler to the correct texture unit
            shader.setInt(name + number, i);
            // and finally bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        
        shader.setBool("hasEmissionTexture", hasEmissionTexture);
        shader.setBool("hasDiffuseTexture", hasDiffuseTexture);
        shader.setBool("hasSpecularTexture", hasSpecularTexture);
        shader.setBool("hasNormalTexture", hasNormalTexture);
        shader.setBool("hasMetallicTexture", hasMetallicTexture);
        shader.setBool("hasRoughnessTexture", hasRoughnessTexture);
        shader.setBool("hasAOTexture", hasAOTexture);
        shader.setVec3("material_diffuseColor", diffuseColor);
        shader.setFloat("shininess", this->shininess);
        shader.setFloat("transparency", this->opacity);
        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        for(size_t i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glActiveTexture(GL_TEXTURE0);
    }

private:
    // render data 
    GLuint VBO, EBO;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
		// ids
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

		// weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
        glBindVertexArray(0);
    }
};
