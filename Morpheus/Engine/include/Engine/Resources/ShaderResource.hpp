#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ShaderLoader.hpp>

#include "EngineFactory.h"
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {
	class ShaderResource : public IResource {
	private:
		DG::IShader* mShader;
	
	public:
		inline ShaderResource(ResourceManager* manager, DG::IShader* shader) :
			mShader(shader), IResource(manager) {
		}

		inline DG::IShader* GetShader() {
			return mShader;
		}

		void SetShader(DG::IShader* shader) {
			mShader = shader;
		}

		ShaderResource* ToShader() override;
		entt::id_type GetType() const override;

		friend class ResourceCache<ShaderResource>;
	};

	class RawShader {
	private:
		std::string mShaderSource;
		std::string mEntryPoint;
		std::string mName;
		DG::ShaderCreateInfo mCreateInfo;

	public:
		inline RawShader(const ShaderPreprocessorOutput& preprocessorOutput,
			DG::SHADER_TYPE type, const std::string& name, const std::string& entryPoint) :
			mShaderSource(preprocessorOutput.mContent),
			mName(name),
			mEntryPoint(entryPoint) {
			mCreateInfo.Desc.ShaderType = type;
			mCreateInfo.SourceLanguage = DG::SHADER_SOURCE_LANGUAGE_HLSL;
		}

		DG::IShader* SpawnOnGPU(DG::IRenderDevice* device);
	};

	template <>
	struct LoadParams<ShaderResource> {
		std::string mSource;
		bool bCache = false;
		const ShaderPreprocessorConfig* mOverrides = nullptr;
		std::string mName;
		std::string mEntryPoint;
		DG::SHADER_TYPE mShaderType;

		static LoadParams<ShaderResource> FromString(const std::string& str) {
			throw std::runtime_error("ShaderResource does not support loading with string source only!");
		}

		LoadParams(const std::string& source,
			DG::SHADER_TYPE type,
			const std::string& name,
			const ShaderPreprocessorConfig* overrides = nullptr,
			const std::string& entryPoint = "main",
			bool cache = false) :
			mSource(source),
			mShaderType(type),
			mName(name),
			mOverrides(overrides),
			mEntryPoint(entryPoint),
			bCache(cache) {
		}
	};

	template <>
	class ResourceCache<ShaderResource> : public IResourceCache {
	private:
		ShaderLoader mLoader;
		ResourceManager* mManager;
		
	public:
		ResourceCache(ResourceManager* mananger) :
			mLoader(mananger),
			mManager(mananger) {
		}

		// Loads resource and blocks until the resource is loaded
		IResource* Load(const void* params) override;
		// Loads resource and adds resulting task to the specified barrier
		TaskId AsyncLoadDeferred(const void* params,
			ThreadPool* threadPool,
			IResource** output,
			const TaskBarrierCallback& callback = nullptr) override;
		virtual void Add(IResource* resource, const void* params) override;
		virtual void Unload(IResource* resource) override;
		virtual void Clear() override;
	};
}