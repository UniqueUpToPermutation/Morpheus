#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ShaderLoader.hpp>
#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/InputController.hpp>

#include <nlohmann/json.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {

	struct VertexAttributeLayout {
	public:
		int mPosition	= -1;
		int mUV			= -1;
		int mNormal 	= -1;
		int mTangent 	= -1;
		int mBitangent	= -1;

		// If this is -1, then assume dense packing
		int mStride 	= -1; 
	};

	enum class InstancingType {
		// No instancing
		NONE,
		// A float4x4 is written to an instance buffer and passed as input to VS
		INSTANCED_STATIC_TRANSFORMS
	};

	class PipelineResource : public IResource {
	private:
		DG::IPipelineState* mState;
		std::string mSource;
		std::vector<DG::LayoutElement> mVertexLayout;
		VertexAttributeLayout mAttributeLayout;
		InstancingType mInstancingType;

		void Init(DG::IPipelineState* state,
			const std::vector<DG::LayoutElement>& layoutElements,
			VertexAttributeLayout attributeLayout);

	public:
		~PipelineResource();

		inline PipelineResource(ResourceManager* manager) :
			IResource(manager),
			mState(nullptr),
			mInstancingType(InstancingType::INSTANCED_STATIC_TRANSFORMS) {
		}

		inline PipelineResource(ResourceManager* manager, 
			DG::IPipelineState* state,
			const std::vector<DG::LayoutElement>& layoutElements,
			VertexAttributeLayout attributeLayout,
			InstancingType instancingType) : 
			IResource(manager),
			mInstancingType(instancingType),
			mState(state),
			mVertexLayout(layoutElements),
			mAttributeLayout(attributeLayout) {
		}

		inline void SetAll(DG::IPipelineState* state,
			const std::vector<DG::LayoutElement>& layoutElements,
			VertexAttributeLayout attributeLayout,
			InstancingType instancingType) {

			if (mState) {
				mState->Release();
				mState = nullptr;
			}

			mState = state;
			mVertexLayout = layoutElements;
			mInstancingType = instancingType;
			mAttributeLayout = attributeLayout;
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

		inline VertexAttributeLayout GetAttributeLayout() const {
			return mAttributeLayout;
		}

		PipelineResource* ToPipeline() override;

		friend class PipelineLoader;
		friend class ResourceCache<PipelineResource>;
	};

	template <>
	struct LoadParams<PipelineResource> {
		std::string mSource;
		ShaderPreprocessorConfig mOverrides;

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
		void ReadSampleDesc(const nlohmann::json& json, DG::SampleDesc* desc);
		void ReadStencilOpDesc(const nlohmann::json& json, DG::StencilOpDesc* desc);
		std::vector<DG::LayoutElement> ReadLayoutElements(const nlohmann::json& json);
		DG::LayoutElement ReadLayoutElement(const nlohmann::json& json);
		DG::VALUE_TYPE ReadValueType(const nlohmann::json& json);
		VertexAttributeLayout ReadVertexAttributes(const nlohmann::json& json);
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

		void Load(const std::string& source, PipelineResource* into, 
			const ShaderPreprocessorConfig* overrides=nullptr);
		void Load(const nlohmann::json& json, const std::string& path, PipelineResource* into, 
			const ShaderPreprocessorConfig* overrides=nullptr);
		DG::ComputePipelineStateCreateInfo ReadComputeInfo(const nlohmann::json& json);
		DG::GraphicsPipelineStateCreateInfo ReadGraphicsInfo(const nlohmann::json& json, 
			std::vector<DG::LayoutElement>* layoutElements,
			std::vector<DG::ShaderResourceVariableDesc>* variables,
			std::vector<DG::ImmutableSamplerDesc>* immutableSamplers,
			std::vector<char*>* strings);
		DG::IShader* LoadShader(const nlohmann::json& shaderConfig,
			const std::string& path,
			const ShaderPreprocessorConfig* config = nullptr);
		DG::IShader* LoadShader(DG::SHADER_TYPE shaderType,
			const std::string& path,
			const std::string& name,
			const std::string& entryPoint,
			const ShaderPreprocessorConfig* config = nullptr);

		inline ShaderLoader* GetShaderLoader() {
			return &mShaderLoader;
		}
	};

	template <>
	class ResourceCache<PipelineResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, PipelineResource*> mCachedResources;
		std::unordered_map<std::string, factory_func_t> mPipelineFactories;
		std::vector<std::pair<PipelineResource*, LoadParams<PipelineResource>>> mDefferedResources;
		PipelineLoader mLoader;
		ResourceManager* mManager;

		void InitFactories();
		void ActuallyLoad(const std::string& source, PipelineResource* into, 
			const ShaderPreprocessorConfig* overrides=nullptr);

	public:
		inline ResourceCache(ResourceManager* manager) :
			mLoader(manager),
			mManager(manager) {
			InitFactories();
		}
		~ResourceCache();

		inline PipelineLoader* GetLoader() {
			return &mLoader;
		}

		IResource* Load(const void* params) override;
		IResource* DeferredLoad(const void* params) override;
		void ProcessDeferred() override;
		void Add(IResource* resource, const void* params) override;
		void Unload(IResource* resource) override;
		void Clear() override;
	};
}