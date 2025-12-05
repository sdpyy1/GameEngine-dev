#include "hzpch.h"
#include "Material.h"
#include "Hazel/Core/Application.h"
#include <Hazel/Renderer/RenderSystem/RenderManager.h>
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"

namespace GameEngine
{ 
	Material::Material()
	{
		materialID = RENDER_RESOURCEMANAGER->AllocateMaterialID();
		Update();
	}

	Material::Material(bool init)
	{
        if (init) {
            materialID = RENDER_RESOURCEMANAGER->AllocateMaterialID();
            textureDiffuse = RENDER_RESOURCEMANAGER->GetWhiteTexture();
            textureRoughness = RENDER_RESOURCEMANAGER->GetWhiteTexture();
            textureMetallic = RENDER_RESOURCEMANAGER->GetBlackTexture();
            Update();
        }
	}

	Material::~Material()
	{
		// if (APP_RENDERSYSTEM && materialID != 0) RENDER_RESOURCEMANAGER->ReleaseMaterialID(materialID);
	}

	void Material::Update()
	{
        materialInfo = {};
        materialInfo.roughness = roughness;
        materialInfo.metallic = metallic;
        materialInfo.useNormaltexture = useNormalTexture;
        materialInfo.diffuse = diffuse;
        materialInfo.emission = emission;

        if (textureDiffuse)  materialInfo.textureDiffuse = textureDiffuse->GetbindlessID();
        if (textureNormal)   materialInfo.textureNormal = textureNormal->GetbindlessID();
        if (textureRoughness) materialInfo.textureRoughness = textureRoughness->GetbindlessID();
        if (textureMetallic) materialInfo.textureMetallic = textureMetallic->GetbindlessID();
        if (textureEmission) materialInfo.textureEmission = textureEmission->GetbindlessID();

        materialInfo.ints = ints;
        materialInfo.floats = floats;
        materialInfo.colors = colors;
        for (uint32_t i = 0; i < 8; i++)
        {
            if (texture2D[i]) materialInfo.texture2D[i] = texture2D[i]->GetbindlessID();
        }
        for (uint32_t i = 0; i < 4; i++)
        {
            if (textureCube[i]) materialInfo.textureCube[i] = textureCube[i]->GetbindlessID();
            if (texture3D[i]) materialInfo.texture3D[i] = texture3D[i]->GetbindlessID();
        }

        RENDER_RESOURCEMANAGER->SetMaterialInfo(materialInfo, materialID);
	}

	void Material::OnLoadAsset()
	{

	}

	void Material::OnSaveAsset()
	{

	}


}