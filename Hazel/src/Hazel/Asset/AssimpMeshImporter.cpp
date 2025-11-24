#include "hzpch.h"
#include "AssimpMeshImporter.h"
#include "Hazel/utils/AssimpLogStream.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <Hazel/Renderer/old/Texture.h>
#include <Hazel/Renderer/old/Renderer.h>
#include "Hazel/Asset/Model/Material.h"
#include "Model/MaterialAsset.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Asset/TextureImporter.h"
#include <Hazel/Platform/Vulkan/VulkanMaterial.h>
#include "Hazel/Asset/AssimpAnimationImporter.h"
#include "Hazel/Math/Math.h"
namespace GameEngine {
#define MESH_DEBUG_LOG 0

#if MESH_DEBUG_LOG
#define DEBUG_PRINT_ALL_PROPS 1
#define HZ_MESH_LOG(...) HZ_CORE_TRACE_TAG("Mesh", __VA_ARGS__)
#define HZ_MESH_ERROR(...) HZ_CORE_ERROR_TAG("Mesh", __VA_ARGS__)
#else
#define HZ_MESH_LOG(...)
#define HZ_MESH_ERROR(...)
#endif

	static const uint32_t s_MeshImportFlags =
		aiProcess_CalcTangentSpace          // Create binormals/tangents just in case
		| aiProcess_Triangulate             // Make sure we're triangles
		| aiProcess_SortByPType             // Split meshes by primitive type
		| aiProcess_GenNormals              // Make sure we have legit normals
		| aiProcess_GenUVCoords             // Convert UVs if required 
		//		| aiProcess_OptimizeGraph
		| aiProcess_OptimizeMeshes          // Batch draws where possible
		| aiProcess_JoinIdenticalVertices
		| aiProcess_LimitBoneWeights        // If more than N (=4) bone weights, discard least influencing bones and renormalise sum to 1
		| aiProcess_ValidateDataStructure   // Validation
		//| aiProcess_GlobalScale             // e.g. convert cm to m for fbx import (and other formats where cm is native)
		;
	namespace Utils {

		glm::mat4 Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix)
		{
			glm::mat4 result;
			//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
			result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
			result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
			result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
			result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
			return result;
		}
	}
	AssimpMeshImporter::AssimpMeshImporter(const std::filesystem::path& path)
		: m_Path(path)
	{
		AssimpLogStream::Initialize();
	}

	bool AssimpMeshImporter::ImportAnimation(const std::string_view animationName, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask, Scope<Animation>& animation)
	{
		Assimp::Importer importer;
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(m_Path.string(), s_MeshImportFlags);
		if (!scene)
		{
			LOG_ERROR_TAG("Animation", "Failed to load mesh source file: {0}", m_Path.string());
			return false;
		}
		// ֻ����ָ������
		uint32_t animationIndex = AssimpAnimationImporter::GetAnimationIndex(scene, animationName);

		if (animationIndex == ~0)
		{
			LOG_ERROR_TAG("Animation", "Animation '{0}' not found in mesh source file: {1}", animationName, m_Path.string());
			return false;
		}

		animation = AssimpAnimationImporter::ImportAnimation(scene, animationIndex, skeleton, isMaskedRootMotion, rootTranslationMask, rootRotationMask);
		return true;
	}


	Ref<MeshSource> AssimpMeshImporter::ImportToMeshSource()
	{
		Ref<MeshSource> meshSource = Ref<MeshSource>::Create();
		meshSource->m_FilePath = m_Path;
		LOG_INFO_TAG("Mesh", "Loading mesh: {0}", m_Path.string());

		Assimp::Importer importer;
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

		const aiScene* scene = importer.ReadFile(m_Path.string(), s_MeshImportFlags);
		if (!scene /* || !scene->HasMeshes()*/)  // note: scene can legit contain no meshes (e.g. it could contain an armature, an animation, and no skin (mesh)))
		{
			LOG_ERROR_TAG("Mesh", "Failed to load mesh file: {0}", m_Path.string());
			meshSource->SetFlag(AssetFlag::Invalid);
			return nullptr;
		}

		LOG_TRACE_TAG("Skeletion","Skeleton {0} found in mesh file '{1}'", meshSource->HasSkeleton() ? "" : "Not found Skeleton", m_Path.string());

		meshSource->m_AnimationNames = AssimpAnimationImporter::GetAnimationNames(scene);
		meshSource->m_Animations = std::vector<Scope<Animation>>(meshSource->m_AnimationNames.size());

		if (scene->HasMeshes())
		{
			uint32_t vertexCount = 0;
			uint32_t indexCount = 0;

			meshSource->m_BoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
			meshSource->m_BoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

			meshSource->m_Submeshes.reserve(scene->mNumMeshes);
			LOG_WARN("ģ�͹� [{}] �� subMesh", scene->mNumMeshes);
			for (unsigned m = 0; m < scene->mNumMeshes; m++)
			{
				LOG_TRACE("��ʼ����subMesh{}", m);
				aiMesh* mesh = scene->mMeshes[m];

				if (!mesh->HasPositions())
				{
					LOG_ERROR("Mesh index {0} with name '{1}' has no vertex positions - skipping import!", m, mesh->mName.C_Str());
				}
				if (!mesh->HasNormals())
				{
					LOG_ERROR("Mesh index {0} with name '{1}' has no vertex normals, and they could not be computed - skipping import!", m, mesh->mName.C_Str());
				}

				bool skip = !mesh->HasPositions() || !mesh->HasNormals();

				// still have to create a placeholder submesh even if we are skipping it (otherwise TraverseNodes() does not work)
				Submesh& submesh = meshSource->m_Submeshes.emplace_back();
				submesh.BaseVertex = vertexCount;
				submesh.BaseIndex = indexCount;
				submesh.MaterialIndex = mesh->mMaterialIndex;
				submesh.VertexCount = skip ? 0 : mesh->mNumVertices;
				submesh.IndexCount = skip ? 0 : mesh->mNumFaces * 3;
				submesh.MeshName = mesh->mName.C_Str();

				if (skip) continue;

				vertexCount += mesh->mNumVertices;
				indexCount += submesh.IndexCount;

				// Vertices
				auto& aabb = submesh.BoundingBox;
				aabb.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
				aabb.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
				LOG_TRACE("���ض����� = {}", mesh->mNumVertices);
				for (size_t i = 0; i < mesh->mNumVertices; i++)
				{
					Vertex vertex;
					vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
					vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
					aabb.Min.x = glm::min(vertex.Position.x, aabb.Min.x);
					aabb.Min.y = glm::min(vertex.Position.y, aabb.Min.y);
					aabb.Min.z = glm::min(vertex.Position.z, aabb.Min.z);
					aabb.Max.x = glm::max(vertex.Position.x, aabb.Max.x);
					aabb.Max.y = glm::max(vertex.Position.y, aabb.Max.y);
					aabb.Max.z = glm::max(vertex.Position.z, aabb.Max.z);

					if (mesh->HasTangentsAndBitangents())
					{
						vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
						vertex.Binormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
					}

					if (mesh->HasTextureCoords(0))
						vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

					meshSource->m_Vertices.push_back(vertex);
				}
				// Indices
				for (size_t i = 0; i < mesh->mNumFaces; i++)
				{
					// we're using aiProcess_Triangulate so this should always be true
					ASSERT(mesh->mFaces[i].mNumIndices == 3, "Must have 3 indices.");
					Index index = { mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] };
					meshSource->m_Indices.push_back(index);

					meshSource->m_TriangleCache[m].emplace_back(meshSource->m_Vertices[index.V1 + submesh.BaseVertex], meshSource->m_Vertices[index.V2 + submesh.BaseVertex], meshSource->m_Vertices[index.V3 + submesh.BaseVertex]);
				}
				LOG_TRACE("���������� = {}", mesh->mNumFaces * 3);

			}
			LOG_INFO("�������ݼ�����ɣ��� [{0}] ���㣬��[{1}] ����", vertexCount,indexCount);

			LOG_TRACE("��ʼ����SubMesh�任��Ϣ");
			MeshNode& rootNode = meshSource->m_Nodes.emplace_back();
			TraverseNodes(meshSource, scene->mRootNode, 0);
			LOG_TRACE("��ʼ����Mesh��AABB��Χ��");
			for (const auto& submesh : meshSource->m_Submeshes)
			{
				AABB transformedSubmeshAABB = submesh.BoundingBox;
				glm::vec3 min = glm::vec3(submesh.Transform * glm::vec4(transformedSubmeshAABB.Min, 1.0f));
				glm::vec3 max = glm::vec3(submesh.Transform * glm::vec4(transformedSubmeshAABB.Max, 1.0f));

				meshSource->m_BoundingBox.Min.x = glm::min(meshSource->m_BoundingBox.Min.x, min.x);
				meshSource->m_BoundingBox.Min.y = glm::min(meshSource->m_BoundingBox.Min.y, min.y);
				meshSource->m_BoundingBox.Min.z = glm::min(meshSource->m_BoundingBox.Min.z, min.z);
				meshSource->m_BoundingBox.Max.x = glm::max(meshSource->m_BoundingBox.Max.x, max.x);
				meshSource->m_BoundingBox.Max.y = glm::max(meshSource->m_BoundingBox.Max.y, max.y);
				meshSource->m_BoundingBox.Max.z = glm::max(meshSource->m_BoundingBox.Max.z, max.z);
			}
		}
		// skinning weights
		meshSource->m_Skeleton = AssimpAnimationImporter::ImportSkeleton(scene);
		if (meshSource->HasSkeleton())
		{
			LOG_INFO_TAG("Mesh", "��ʼ����������Ϣ");
			meshSource->m_BoneInfluences.resize(meshSource->m_Vertices.size());
			for (uint32_t m = 0; m < scene->mNumMeshes; m++)
			{
				aiMesh* mesh = scene->mMeshes[m];
				Submesh& submesh = meshSource->m_Submeshes[m];

				if (mesh->mNumBones > 0)
				{
					submesh.IsRigged = true;
					for (uint32_t i = 0; i < mesh->mNumBones; i++)
					{
						aiBone* bone = mesh->mBones[i];
						bool hasNonZeroWeight = false;
						for (size_t j = 0; j < bone->mNumWeights; j++)
						{
							if (bone->mWeights[j].mWeight > 0.000001f)
							{
								hasNonZeroWeight = true;
								break;
							}
						}
						if (!hasNonZeroWeight)
							continue;

						// Find bone in skeleton
						uint32_t boneIndex = meshSource->m_Skeleton->GetBoneIndex(bone->mName.C_Str());
						if (boneIndex == Skeleton::NullIndex)
						{
							LOG_ERROR_TAG("Animation", "Could not find mesh bone '{}' in skeleton!", bone->mName.C_Str());
						}

						uint32_t boneInfoIndex = ~0;
						for (size_t j = 0; j < meshSource->m_BoneInfo.size(); ++j)
						{
							if (meshSource->m_BoneInfo[j].BoneIndex == boneIndex)
							{
								boneInfoIndex = static_cast<uint32_t>(j);
								break;
							}
						}
						if (boneInfoIndex == ~0)
						{
							boneInfoIndex = static_cast<uint32_t>(meshSource->m_BoneInfo.size());
							const auto& boneInfo = meshSource->m_BoneInfo.emplace_back(Utils::Mat4FromAIMatrix4x4(bone->mOffsetMatrix), boneIndex);
	/*						LOG_INFO_TAG("Mesh", "BoneInfo for bone '{0}'", bone->mName.C_Str());
							LOG_INFO_TAG("Mesh", "  SubMeshIndex = {0}", m);
							LOG_INFO_TAG("Mesh", "  BoneIndex = {0}", boneIndex);*/
							glm::vec3 translation;
							glm::quat rotationQuat;
							glm::vec3 scale;
							Math::DecomposeTransform(boneInfo.InverseBindPose, translation, rotationQuat, scale);
							glm::vec3 rotation = glm::degrees(glm::eulerAngles(rotationQuat));
							//LOG_INFO_TAG("Mesh", "  Inverse Bind Pose = {");
							//LOG_INFO("    translation: ({0:8.4f}, {1:8.4f}, {2:8.4f})", translation.x, translation.y, translation.z);
							//LOG_INFO("    rotation:    ({0:8.4f}, {1:8.4f}, {2:8.4f})", rotation.x, rotation.y, rotation.z);
							//LOG_INFO("    scale:       ({0:8.4f}, {1:8.4f}, {2:8.4f})", scale.x, scale.y, scale.z);
							//LOG_INFO("  }");
						}

						for (size_t j = 0; j < bone->mNumWeights; j++)
						{
							int VertexID = submesh.BaseVertex + bone->mWeights[j].mVertexId;
							float Weight = bone->mWeights[j].mWeight;
							meshSource->m_BoneInfluences[VertexID].AddBoneData(boneInfoIndex, Weight);
						}
					}
				}
			}

			for (auto& boneInfluence : meshSource->m_BoneInfluences)
			{
				boneInfluence.NormalizeWeights();
			}
			LOG_TRACE("��[{}]������", meshSource->m_BoneInfo.size());
		}

		// Materials
		Ref<Texture2D> whiteTexture = Renderer::GetWhiteTexture();
		if (scene->HasMaterials())
		{
			LOG_INFO("����[{}]�ֲ���", scene->mNumMaterials);
			meshSource->m_Materials.resize(scene->mNumMaterials);
			// ����ÿ�ֲ��ʣ�ÿ�ֲ��ʶ�����������ͼ�����ݣ�
			for (uint32_t i = 0; i < scene->mNumMaterials; i++)
			{
				auto aiMaterial = scene->mMaterials[i];
				auto aiMaterialName = aiMaterial->GetName();
				LOG_TRACE("��ʼ��������[{}]", aiMaterialName.data);

				// �������ʶ�����Ҫһ��Shader������
				Ref<MaterialOld> material = MaterialOld::Create(Renderer::GetShaderLibrary()->Get("gBuffer"), aiMaterialName.data);
				auto ma = Ref<MaterialAsset>::Create(material);

				aiString aiTexPath;
				// ����ĳ�������ǹ̶�ֵ
				glm::vec3 albedoColor(0.8f);
				glm::vec3 emission(0.0f);
				aiColor3D aiColor(1.0f), aiEmission(0.0f);
				if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS)
					albedoColor = { aiColor.r, aiColor.g, aiColor.b };

				if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmission) == AI_SUCCESS)
					emission = { aiEmission.r, aiEmission.g ,aiEmission.b };

				ma->SetAlbedoColor(albedoColor);
				ma->SetEmission(emission);

				float roughness, metalness;
				if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn_SUCCESS)
					roughness = 0.4f; // Default value

				if (aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness) != aiReturn_SUCCESS)
					metalness = 0.0f;

				// Physically realistic materials are either metal (1.0) or not (0.0)
				// Some models seem to come in with 0.5 which seems wrong - materials are either metal or they are not.
				// (maybe these are specular workflow, and what we're seeing is specular = 0.5 in AI_MATKEY_REFLECTIVITY (?))
				if (metalness < 0.9f)
					metalness = 0.0f;
				else
					metalness = 1.0f;

				ma->SetRoughness(roughness);
				ma->SetMetalness(metalness);

				LOG_INFO("    COLOR = {0}, {1}, {2}", aiColor.r, aiColor.g, aiColor.b);
				LOG_INFO("    ROUGHNESS = {0}", roughness);
				LOG_INFO("    METALNESS = {0}", metalness);
				// ����ĳ����������ͼ
				bool hasAlbedoMap = aiMaterial->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &aiTexPath) == AI_SUCCESS;
				if (!hasAlbedoMap)
				{
					// no PBR base color. Try old-school diffuse  (note: should probably combine with specular in this case)
					hasAlbedoMap = aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;
				}
				if (hasAlbedoMap)
				{
					AssetHandle textureHandle = 0;
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					spec.Format = ImageFormat::SRGBA;
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						textureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, Buffer(aiTexEmbedded->pcData, 1)));
					}
					else
					{
						// TODO: Temp - this should be handled by GameEngine's filesystem
						// NOTE(Yan): we probably shouldn't make this a memory-only asset, since this
						//            should already exist within the asset registry as a texture asset
						auto parentPath = m_Path.parent_path();
						auto texturePath = parentPath / aiTexPath.C_Str();
						if (!std::filesystem::exists(texturePath))
						{
							LOG_INFO("    Albedo map path = {0} --> NOT FOUND", texturePath);
							texturePath = parentPath / texturePath.filename();
						}
						LOG_INFO("����Albedo��ͼ path = {0}{1}", texturePath, std::filesystem::exists(texturePath) ? "" : " --> NOT FOUND");
						textureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, texturePath));
					}

					ma->SetAlbedoMap(textureHandle);
					ma->SetAlbedoColor(glm::vec3(1.0f));
				}



				// EMISSION map
				bool hasEmissiveMap = aiMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &aiTexPath) == AI_SUCCESS;
				if (hasEmissiveMap) {
					AssetHandle textureHandle = 0;
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					spec.Format = ImageFormat::SRGBA;
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Format = ImageFormat::RGBA;
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						textureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, Buffer(aiTexEmbedded->pcData, 1)));
					}
					else
					{
						// TODO: Temp - this should be handled by GameEngine's filesystem
						auto parentPath = m_Path.parent_path();
						auto texturePath = parentPath / aiTexPath.C_Str();
						if (!std::filesystem::exists(texturePath))
						{
							LOG_INFO("    Emissive map path = {0} --> NOT FOUND", texturePath);
							texturePath = parentPath / texturePath.filename();
						}
						LOG_INFO("���� Emissive ��ͼ path = {0}{1}", texturePath, std::filesystem::exists(texturePath) ? "" : " --> NOT FOUND");
						textureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, texturePath));
					}
					ma->SetEmissiveMap(textureHandle);

				}

				// Normal maps
				bool hasNormalMap = aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS;
				if (hasNormalMap)
				{
					AssetHandle textureHandle = 0;

					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Format = ImageFormat::RGBA;
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						textureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, Buffer(aiTexEmbedded->pcData, 1)));
					}
					else
					{
						// TODO: Temp - this should be handled by GameEngine's filesystem
						auto parentPath = m_Path.parent_path();
						auto texturePath = parentPath / aiTexPath.C_Str();
						if (!std::filesystem::exists(texturePath))
						{
							LOG_INFO("    Normal map path = {0} --> NOT FOUND", texturePath);
							texturePath = parentPath / texturePath.filename();
						}
						LOG_INFO("���� Normal ��ͼ path = {0}{1}", texturePath, std::filesystem::exists(texturePath) ? "" : " --> NOT FOUND");
						textureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, texturePath));
					}

					ma->SetNormalMap(textureHandle);
					ma->SetUseNormalMap(true);

				}

				// Roughness map
				bool hasRoughnessMap = aiMaterial->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &aiTexPath) == AI_SUCCESS;
				bool invertRoughness = false;
				if (!hasRoughnessMap)
				{
					// no PBR roughness. Try old-school shininess.  (note: this also picks up the gloss texture from PBR specular/gloss workflow).
					// Either way, Roughness = (1 - shininess)
					hasRoughnessMap = aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) == AI_SUCCESS;
					invertRoughness = true;
				}

				AssetHandle roughnessTextureHandle = 0;
				if (hasRoughnessMap)
				{
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Format = ImageFormat::RGBA;
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						aiTexel* texels = aiTexEmbedded->pcData;
						if (invertRoughness)
						{
							if (spec.Height == 0)
							{
								auto buffer = TextureImporter::ToBufferFromMemory(Buffer(aiTexEmbedded->pcData, spec.Width), spec.Format, spec.Width, spec.Height);
								texels = (aiTexel*)buffer.Data;
							}
							for (uint32_t i = 0; i < spec.Width * spec.Height; ++i)
							{
								auto& texel = texels[i];
								texel.r = 255 - texel.r;
								texel.g = 255 - texel.g;
								texel.b = 255 - texel.b;
							}
						}
						roughnessTextureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, Buffer(texels, 1)));
					}
					else
					{
						// TODO: Temp - this should be handled by GameEngine's filesystem
						auto parentPath = m_Path.parent_path();
						auto texturePath = parentPath / aiTexPath.C_Str();
						if (!std::filesystem::exists(texturePath))
						{
							LOG_INFO("    Roughness map path = {0} --> NOT FOUND", texturePath);
							texturePath = parentPath / texturePath.filename();
						}
						LOG_INFO("����Roughness ��ͼ path = {0}{1}", texturePath, std::filesystem::exists(texturePath) ? "" : " --> NOT FOUND");
						auto buffer = TextureImporter::ToBufferFromFile(texturePath, spec.Format, spec.Width, spec.Height);
						aiTexel* texels = (aiTexel*)buffer.Data;
						if (invertRoughness)
						{
							for (uint32_t i = 0; i < spec.Width * spec.Height; i += 4)
							{
								aiTexel& texel = texels[i];
								texel.r = 255 - texel.r;
								texel.g = 255 - texel.g;
								texel.b = 255 - texel.b;
							}
						}
						roughnessTextureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, buffer));
					}

					ma->SetRoughnessMap(roughnessTextureHandle);
					ma->SetRoughness(1.0f);
				}

				// Metalness map
				bool hasMetalnessMap = aiMaterial->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &aiTexPath) == AI_SUCCESS;
				AssetHandle metalnessTextureHandle = 0;
				if (hasMetalnessMap)
				{
					TextureSpecification spec;
					spec.DebugName = aiTexPath.C_Str();
					if (auto aiTexEmbedded = scene->GetEmbeddedTexture(aiTexPath.C_Str()))
					{
						spec.Format = ImageFormat::RGB;
						spec.Width = aiTexEmbedded->mWidth;
						spec.Height = aiTexEmbedded->mHeight;
						metalnessTextureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, Buffer(aiTexEmbedded->pcData, 1)));
					}
					else
					{
						auto parentPath = m_Path.parent_path();
						auto texturePath = parentPath / aiTexPath.C_Str();
						if (!std::filesystem::exists(texturePath))
						{
							LOG_INFO("    Roughness map path = {0} --> NOT FOUND", texturePath);
							texturePath = parentPath / texturePath.filename();
						}
						LOG_INFO("����Metalness��ͼ path = {0}{1}", texturePath, std::filesystem::exists(texturePath) ? "" : " --> NOT FOUND");
						metalnessTextureHandle = AssetManager::AddMemoryOnlyAsset(Texture2D::Create(spec, texturePath));
					}

					ma->SetMetalnessMap(metalnessTextureHandle);
					ma->SetMetalness(1.0f);
					if (Renderer::Current() == RendererAPI::Type::Vulkan) {
						ma->GetMaterial().As<VulkanMaterial>()->UpdateDescriptorSet(true);
					}

				}

				AssetHandle maHandle = AssetManager::AddMemoryOnlyAsset(ma);
				meshSource->m_Materials[i] = maHandle;

			}
			HZ_MESH_LOG("------------------------");
		}
		else
		{
			// û�в��ʣ�����Ĭ�ϵ�
			if (scene->HasMeshes())
			{
				Ref<MaterialOld> material = MaterialOld::Create(Renderer::GetShaderLibrary()->Get("gBuffer"), "GameEngine-Default");
				AssetHandle maHandle = AssetManager::AddMemoryOnlyAsset(Ref<MaterialAsset>::Create(material));
				meshSource->m_Materials.push_back(maHandle);
			}
		}

		if (meshSource->m_Vertices.size())
		{
			LOG_INFO("ģ�͹�{}����", meshSource->m_Vertices.size());
			meshSource->m_VertexBuffer = VertexBuffer::Create(meshSource->m_Vertices.data(), (uint32_t)(meshSource->m_Vertices.size() * sizeof(Vertex)),"VertexBuffer");
		}
		if (meshSource->m_BoneInfluences.size() > 0)
		{
			meshSource->m_BoneInfluenceBuffer = VertexBuffer::Create(meshSource->m_BoneInfluences.data(), (uint32_t)(meshSource->m_BoneInfluences.size() * sizeof(BoneInfluence)),"BoneInfluenceBuffer");
		}

		if (meshSource->m_Indices.size())
			LOG_INFO("ģ�͹�{}����", meshSource->m_Indices.size()*3);
			meshSource->m_IndexBuffer = IndexBuffer::Create(meshSource->m_Indices.data(), (uint32_t)(meshSource->m_Indices.size() * sizeof(Index)));

		return meshSource;
	}
	void AssimpMeshImporter::TraverseNodes(Ref<MeshSource> meshSource, void* assimpNode, uint32_t nodeIndex, const glm::mat4& parentTransform, uint32_t level)
	{
		aiNode* aNode = (aiNode*)assimpNode;

		MeshNode& node = meshSource->m_Nodes[nodeIndex];
		node.Name = aNode->mName.C_Str();
		node.LocalTransform = Utils::Mat4FromAIMatrix4x4(aNode->mTransformation);

		glm::mat4 transform = parentTransform * node.LocalTransform;
		for (uint32_t i = 0; i < aNode->mNumMeshes; i++)
		{
			uint32_t submeshIndex = aNode->mMeshes[i];
			auto& submesh = meshSource->m_Submeshes[submeshIndex];
			submesh.NodeName = aNode->mName.C_Str();
			submesh.Transform = transform;
			submesh.LocalTransform = node.LocalTransform;

			node.Submeshes.push_back(submeshIndex);
		}

		// HZ_MESH_LOG("{0} {1}", LevelToSpaces(level), node->mName.C_Str());

		uint32_t parentNodeIndex = (uint32_t)meshSource->m_Nodes.size() - 1;
		node.Children.resize(aNode->mNumChildren);
		for (uint32_t i = 0; i < aNode->mNumChildren; i++)
		{
			MeshNode& child = meshSource->m_Nodes.emplace_back();
			uint32_t childIndex = static_cast<uint32_t>(meshSource->m_Nodes.size()) - 1;
			child.Parent = parentNodeIndex;
			meshSource->m_Nodes[nodeIndex].Children[i] = childIndex;
			TraverseNodes(meshSource, aNode->mChildren[i], childIndex, transform, level + 1);
		}
	}
}
