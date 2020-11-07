#pragma once

#include <Engine/Resource.hpp>
#include <nlohmann/json.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "InputController.hpp"
#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {
	class PipelineResource : public Resource {
	private:
		DG::IPipelineState* mState;
		std::string mSource;

	public:
		~PipelineResource();

		inline PipelineResource(ResourceManager* manager, 
			DG::IPipelineState* state) : Resource(manager),
			mState(state) {
		}

		inline DG::IPipelineState* GetState() {
			return mState;
		}

		inline const DG::IPipelineState* GetState() const {
			return mState;
		}

		inline std::string GetSource() const {
			return mSource;
		}

		virtual entt::id_type GetType() const {
			return resource_type::type<PipelineResource>;
		}

		friend class PipelineLoader;
		friend class ResourceCache<PipelineResource>;
	};

	template <>
	struct LoadParams<PipelineResource> {
		std::string mSource;

		static LoadParams<PipelineResource> FromString(const std::string& str) {
			LoadParams<PipelineResource> p;
			p.mSource = str;
			return p;
		}
	};

	class PipelineLoader {
	private:
		ResourceManager* mManager;

	public:
		inline PipelineLoader(ResourceManager* manager) :
			mManager(manager) {
		}

		DG::TEXTURE_FORMAT ReadTextureFormat(const std::string& str);
		DG::PRIMITIVE_TOPOLOGY ReadPrimitiveTopology(const std::string& str);
		void ReadRasterizerDesc(const nlohmann::json& json, DG::RasterizerStateDesc* desc);
		void ReadDepthStencilDesc(const nlohmann::json& json, DG::DepthStencilStateDesc* desc);
		DG::CULL_MODE ReadCullMode(const std::string& str);
		DG::FILL_MODE ReadFillMode(const std::string& str);
		DG::STENCIL_OP ReadStencilOp(const std::string& str);
		DG::COMPARISON_FUNCTION ReadComparisonFunc(const std::string& str);
		void ReadStencilOpDesc(const nlohmann::json& json, DG::StencilOpDesc* desc);
		DG::SHADER_TYPE ReadShaderType(const std::string& str);

		PipelineResource* Load(const std::string& source);
		PipelineResource* Load(const nlohmann::json& json, const std::string& path);
		DG::ComputePipelineStateCreateInfo ReadComputeInfo(const nlohmann::json& json);
		DG::GraphicsPipelineStateCreateInfo ReadGraphicsInfo(const nlohmann::json& json);
		DG::IShader* LoadShader(const nlohmann::json& shaderConfig,
			const std::string& path);
	};

	template <>
	class ResourceCache<PipelineResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, PipelineResource*> mCachedResources;
		PipelineLoader mLoader;
		ResourceManager* mManager;

	public:
		inline ResourceCache(ResourceManager* manager) :
			mLoader(manager),
			mManager(manager) {
		}
		~ResourceCache();

		Resource* Load(const void* params) override;
		void Add(Resource* resource, const void* params) override;
		void Unload(Resource* resource) override;
	};
}