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

		struct Data {
			std::unique_ptr<RawShader> mRawShader;
		};

		Task task([params = *params_cast, renderDevice, overrides, fileLoader, shaderLoader, resource, data = Data()](const TaskParams& e) mutable {
			if (e.mTask->SubTask()) {
				ShaderPreprocessorOutput output;
				shaderLoader->Load(params.mSource, fileLoader, &output, &overrides);

				data.mRawShader = std::make_unique<RawShader>(output, params.mShaderType, params.mName, params.mEntryPoint);

				if (e.mTask->RequestThreadSwitch(e, ASSIGN_THREAD_MAIN))
					return TaskResult::REQUEST_THREAD_SWITCH;
			}

			if (e.mTask->SubTask()) {
				DG::IShader* shader = data.mRawShader->SpawnOnGPU(renderDevice);
				resource->SetShader(shader);
				resource->SetLoaded(true);
			}

			return TaskResult::FINISHED;
		}, 
		std::string("Load Shader ") + params_cast->mSource,
		TaskType::FILE_IO);

		resource->mBarrier.mIn.Lock().Connect(&task->Out());

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