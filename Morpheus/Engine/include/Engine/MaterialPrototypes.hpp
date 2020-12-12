#pragma once

#include "BasicMath.hpp"
#include "RenderDevice.h"

using float2 = Diligent::float2;
using float3 = Diligent::float3;
using float4 = Diligent::float4;
using float4x4 = Diligent::float4x4;

#include <shaders/PBRStructures.hlsl>
#include <nlohmann/json.hpp>

#include <Engine/Resource.hpp>

namespace DG = Diligent;

namespace Morpheus {

	class MaterialResource;
	class ResourceManager;
	class PipelineResource;
	class TextureResource;
	class MaterialPrototype;

	typedef std::function<MaterialPrototype*(
		ResourceManager* manager,
		const std::string&,
		const std::string&,
		const nlohmann::json& config)> prototype_spawner_t;

	class MaterialPrototype {
	protected:
		void InternalInitialize(MaterialResource* material,
			DG::IShaderResourceBinding* binding, 
			PipelineResource* pipeline,
			const std::vector<TextureResource*>& textures,
			const std::vector<DG::IBuffer*>& buffers);

	public:
		virtual ~MaterialPrototype() {}
		virtual void InitializeMaterial(
			ResourceManager* manager,
			ResourceCache<MaterialResource>* cache, 
			MaterialResource* into) = 0;
		virtual MaterialPrototype* DeepCopy() const = 0;
	};

	template <typename T>
	MaterialPrototype* AbstractConstructor(
		ResourceManager* manager,
		const std::string& source, 
		const std::string& path,
		const nlohmann::json& config) {
		return new T(manager, source, path, config);
	}

	class MaterialPrototypeFactory {
	private:
		std::unordered_map<std::string, prototype_spawner_t> mMap;

	public:
		MaterialPrototypeFactory();
		MaterialPrototype* Spawn(
			const std::string& type,
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config) const;
	};

	void LoadPBRShaderInfo(const nlohmann::json& json, GLTFMaterialShaderInfo* result);

	class JsonMaterialPrototype : public MaterialPrototype {
	private:
	PipelineResource* mPipeline;
		std::vector<DG::Uint32> mVariableIndices;
		std::vector<TextureResource*> mTextures;

	public:
		JsonMaterialPrototype(
			const JsonMaterialPrototype& other);
		JsonMaterialPrototype(
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config);
		~JsonMaterialPrototype();

		void InitializeMaterial(
			ResourceManager* manager,
			ResourceCache<MaterialResource>* cache,
			MaterialResource* into) override;
		MaterialPrototype* DeepCopy() const override;
	};

	class StaticMeshPBRMaterialPrototype : public MaterialPrototype {
	private:
		TextureResource* mAlbedo;
		TextureResource* mRoughness;
		TextureResource* mMetallic;
		TextureResource* mNormal;
		TextureResource* mAO;
		TextureResource* mEmissive;
		PipelineResource* mPipeline;
		GLTFMaterialShaderInfo mMaterialInfo;

		DG::IBuffer* CreateMaterialInfoBuffer(
			const GLTFMaterialShaderInfo& info,
			ResourceManager* manager);

	public:
		StaticMeshPBRMaterialPrototype(
			const StaticMeshPBRMaterialPrototype& other);
		StaticMeshPBRMaterialPrototype(
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config);
		StaticMeshPBRMaterialPrototype(
			PipelineResource* pipeline,
			const GLTFMaterialShaderInfo& info,
			TextureResource* albedo,
			TextureResource* roughness,
			TextureResource* metallic,
			TextureResource* normal,
			TextureResource* ao = nullptr,
			TextureResource* emissive = nullptr);
		~StaticMeshPBRMaterialPrototype();

		void InitializeMaterial(
			ResourceManager* manager,
			ResourceCache<MaterialResource>* cache,
			MaterialResource* into) override;
		MaterialPrototype* DeepCopy() const override;
	};

	class BasicTexturedMaterialPrototype : public MaterialPrototype {
	private:
		TextureResource* mColor;
		PipelineResource* mPipeline;

	public:
		BasicTexturedMaterialPrototype(
			const BasicTexturedMaterialPrototype& other);

		BasicTexturedMaterialPrototype(
			ResourceManager* manager,
			const std::string& source, 
			const std::string& path,
			const nlohmann::json& config);
		BasicTexturedMaterialPrototype(
			PipelineResource* pipeline,
			TextureResource* color);
		~BasicTexturedMaterialPrototype();

		void InitializeMaterial(
			ResourceManager* manager,
			ResourceCache<MaterialResource>* cache,
			MaterialResource* into) override;
		MaterialPrototype* DeepCopy() const override;
	};
}