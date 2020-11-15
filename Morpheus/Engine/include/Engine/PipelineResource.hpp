#pragma once

#include <Engine/Resource.hpp>
#include <Engine/ShaderLoader.hpp>
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

	struct VertexAttributeIndices {
	public:
		int mPosition	= -1;
		int mUV			= -1;
		int mNormal 	= -1;
		int mTangent 	= -1;
		int mBitangent	= -1;
	};

	class PipelineResource : public Resource {
	private:
		DG::IPipelineState* mState;
		std::string mSource;
		std::vector<DG::LayoutElement> mVertexLayout;
		VertexAttributeIndices mAttributeIndices;

		void Init(DG::IPipelineState* state,
			std::vector<DG::LayoutElement> layoutElements,
			VertexAttributeIndices attributeIndices);

	public:
		~PipelineResource();

		inline PipelineResource(ResourceManager* manager) :
			Resource(manager),
			mState(nullptr) {
		}

		inline PipelineResource(ResourceManager* manager, 
			DG::IPipelineState* state,
			std::vector<DG::LayoutElement> layoutElements,
			VertexAttributeIndices attributeIndices) : 
			Resource(manager) {
			Init(state, layoutElements, attributeIndices);
		}

		inline bool IsReady() const {
			return mState != nullptr;
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

		entt::id_type GetType() const override {
			return resource_type::type<PipelineResource>;
		}

		inline std::vector<DG::LayoutElement> GetVertexLayout() const {
			return mVertexLayout;
		}

		inline VertexAttributeIndices GetAttributeIndices() const {
			return mAttributeIndices;
		}

		PipelineResource* ToPipeline() override;

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

	DG::SHADER_TYPE ReadShaderType(const std::string& str);

	class PipelineLoader {
	private:
		ResourceManager* mManager;
		ShaderLoader mShaderLoader;

	public:
		inline PipelineLoader(ResourceManager* manager) :
			mManager(manager), 
			mShaderLoader(manager) {
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
		std::vector<DG::LayoutElement> ReadLayoutElements(const nlohmann::json& json);
		DG::LayoutElement ReadLayoutElement(const nlohmann::json& json);
		DG::VALUE_TYPE ReadValueType(const nlohmann::json& json);
		VertexAttributeIndices ReadVertexAttributes(const nlohmann::json& json);
		DG::SHADER_RESOURCE_VARIABLE_TYPE ReadShaderResourceVariableType(const nlohmann::json& json);
		DG::PipelineResourceLayoutDesc ReadResourceLayout(const nlohmann::json& json,
			std::vector<DG::ShaderResourceVariableDesc>* variables,
			std::vector<DG::ImmutableSamplerDesc>* immutableSamplers,
			std::vector<char*>* strings);
		DG::SamplerDesc ReadSamplerDesc(const nlohmann::json& json);
		DG::SHADER_TYPE ReadShaderStages(const nlohmann::json& json);
		DG::TEXTURE_ADDRESS_MODE ReadTextureAddressMode(const nlohmann::json& json);
		DG::FILTER_TYPE ReadFilterType(const nlohmann::json& json);
		DG::INPUT_ELEMENT_FREQUENCY ReadInputElementFrequency(const std::string& str);

		void Load(const std::string& source, PipelineResource* into);
		void Load(const nlohmann::json& json, const std::string& path, PipelineResource* into);
		DG::ComputePipelineStateCreateInfo ReadComputeInfo(const nlohmann::json& json);
		DG::GraphicsPipelineStateCreateInfo ReadGraphicsInfo(const nlohmann::json& json, 
			std::vector<DG::LayoutElement>* layoutElements,
			std::vector<DG::ShaderResourceVariableDesc>* variables,
			std::vector<DG::ImmutableSamplerDesc>* immutableSamplers,
			std::vector<char*>* strings);
		DG::IShader* LoadShader(const nlohmann::json& shaderConfig,
			const std::string& path);
	};

	template <>
	class ResourceCache<PipelineResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, PipelineResource*> mCachedResources;
		std::vector<std::pair<PipelineResource*, LoadParams<PipelineResource>>> mDefferedResources;
		PipelineLoader mLoader;
		ResourceManager* mManager;

	public:
		inline ResourceCache(ResourceManager* manager) :
			mLoader(manager),
			mManager(manager) {
		}
		~ResourceCache();

		Resource* Load(const void* params) override;
		Resource* DeferredLoad(const void* params) override;
		void ProcessDeferred() override;
		void Add(Resource* resource, const void* params) override;
		void Unload(Resource* resource) override;
		void Clear() override;
	};
}