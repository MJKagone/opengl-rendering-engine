#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model
{
	public:
		Model(char* path)
		{
			loadModel(path);
		}
		void Draw(Shader& shader);

	private:
		// Model data
		vector<Mesh> meshes;
		string directory;
		
		void loadModel(string path)
		{
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path,
				aiProcess_Triangulate | aiProcess_FlipUVs);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << "\n";
				return;
			}
			directory = path.substr(0, path.find_last_of('/'));
			processNode(scene->mRootNode, scene);
		}

		void processNode(aiNode* node, const aiScene* scene)
		{
			for (unsigned int i = 0; i < node->mNumMeshes; i++)
			{
				aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				meshes.push_back(processMesh(mesh, scene));
			}
			for (unsigned int i = 0; i < node->mNumChildren; i++)
			{
				processNode(node->mChildren[i], scene);
			}
		}

		Mesh processMesh(aiMesh* mesh, const aiScene* scene)
		{
		Mesh processMesh(aiMesh *mesh, const aiScene *scene)
		{
			vector<Vertex> vertices;
			vector<unsigned int> indices;
			vector<Texture> textures;
			for(unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
			Vertex vertex;
			// process vertex positions, normals and texture coordinates
			[...]
			vertices.push_back(vertex);
			}
			// process indices
			[...]
			// process material
			if(mesh->mMaterialIndex >= 0)
			{
			[...]
			}
			return Mesh(vertices, indices, textures);
			}

		vector<Texture> loadMaterialTextures(aiMaterial* mat,
		aiTextureType type, string typeName);
};

