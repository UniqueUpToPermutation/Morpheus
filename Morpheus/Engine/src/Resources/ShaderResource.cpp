#include <Engine/Resources/ShaderResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Engine.hpp>

namespace Morpheus {
	ShaderResource* ShaderResource::ToShader() {
		return this;
	}
	
	entt::id_type ShaderResource::GetType() const {
		return resource_type::type<ShaderResource>;
	}

	DG::IShader* RawShader::SpawnOnGPU(DG::IRenderDevice* device) {
		DG::IShader* shader = nullptr;
		mCreateInfo.Source = mShaderSource.c_str();
		mCreateInfo.EntryPoint = mEntryPoint.c_str();
		mCreateInfo.Desc.Name = mName.c_str();
		device->CreateShader(mCreateInfo, &shader);
		return shader;
	}

	DG::IShader* CompileShader(DG::IRenderDevice* device, 
		const ShaderPreprocessorOutput& preprocessorOutput,
		DG::SHADER_TYPE type, 
		const std::string& name, 
		const std::string& entryPoint) {
		RawShader raw(preprocessorOutput, type, name, entryPoint);
		return raw.SpawnOnGPU(device);
	}

	DG::IShader* CompileEmbeddedShader(DG::IRenderDevice* device,
		const std::string& source,
		DG::SHADER_TYPE type, 
		const std::string& name, 
		const std::string& entryPoint,
		const ShaderPreprocessorConfig* config,
		IVirtualFileSystem* fileLoader) {

		ShaderPreprocessor preprocessor;
		ShaderPreprocessorOutput output;
		ShaderPreprocessorConfig defaultConfig;
		preprocessor.Load(source, fileLoader, &output, &defaultConfig, config);
		RawShader raw(output, type, name, entryPoint);
		return raw.SpawnOnGPU(device);
	}

	// Loads resource and adds resulting task to the specified barrier
	Task ResourceCache<ShaderResource>::LoadTask(const void* params,
		IResource** output) {
		auto params_cast = reinterpret_cast<const LoadParams<ShaderResource>*>(params);

		ShaderPreprocessorConfig overrides;
		EmbeddedFileLoader* fileLoader = mManager->GetEmbededFileLoader();
		ShaderLoader* shaderLoader = &mLoader;
		if (params_cast->mOverrides)
			overrides = *params_cast->mOverrides;

		auto resource = new ShaderResource(mManager, nullptr);
		DG::IRenderDevice* renderDevice = mManager->GetParent()->GetDevice();

		Task task;
		task.mSyncPoint = resource->GetLoadBarrier();
		task.mType = TaskType::FILE_IO;
		task.mFunc = [params = *params_cast, renderDevice, overrides, fileLoader, shaderLoader, resource](const TaskParams& e) {
			ShaderPreprocessorOutput output;
			shaderLoader->Load(params.mSource, fileLoader, &output, &overrides);

			auto raw = std::make_unique<RawShader>(output, params.mShaderType, params.mName, params.mEntryPoint);

			e.mQueue->Submit([raw = std::move(raw), renderDevice, resource](const TaskParams& e) mutable {
				DG::IShader* shader = raw->SpawnOnGPU(renderDevice);
				resource->SetShader(shader);
			}, TaskType::RENDER, resource->GetLoadBarrier(), ASSIGN_THREAD_MAIN);
		};

		*output = resource;
		
		return task;
	}

	void ResourceCache<ShaderResource>::Add(IResource* resource, const void* params) {
		throw std::runtime_error("Not implemented yet!");
	}

	void ResourceCache<ShaderResource>::Unload(IResource* resource) {
		auto shader = resource->ToShader();

		if (shader) {
			shader->mShader->Release();
			delete shader;
		}
	}

	void ResourceCache<ShaderResource>::Clear() {
	}
}