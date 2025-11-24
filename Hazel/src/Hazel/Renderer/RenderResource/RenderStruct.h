#pragma once
#include <glm/ext/matrix_float4x4.hpp>
namespace GameEngine {
	namespace V2 {
		struct CameraData {
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 viewproj;
			float Width;
			float Height;
			float Near;
			float Far;
			glm::vec3 Position;
			float padding;
			glm::mat4 InverseViewProj;
		};


        typedef struct MaterialInfo
        {
            float roughness;
            float metallic;
            float alphaClip;
            uint32_t useNormaltexture;

            glm::vec4 diffuse;
            glm::vec4 emission;

            uint32_t textureDiffuse;
            uint32_t textureNormal; 
            uint32_t textureArm;        //AO/Roughness/Metallic
            uint32_t textureSpecular;

            //预留的通用槽位///////////////////////////////
            std::array<int32_t, 8> ints;
            std::array<float, 8> floats;
            std::array<glm::vec4, 8> colors;

            std::array<uint32_t, 8> texture2D;
            std::array<uint32_t, 4> textureCube;
            std::array<uint32_t, 4> texture3D; 

        } MaterialInfo;
	}
}