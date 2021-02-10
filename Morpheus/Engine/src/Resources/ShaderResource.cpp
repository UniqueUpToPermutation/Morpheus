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

	// Loads resource and blocks until the resource is loaded
	IResource* ResourceCache<ShaderResource>::Load(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<ShaderResource>*>(params);
		std::cout << "Loading " << params_cast->mSource << "..." << std::endl;

		ShaderPreprocessorOutput output;
		mLoader.Load(params_cast->mSource, mManager->GetEmbededFileLoader(), &output, params_cast->mOverrides);
		RawShader raw(output, params_cast->mShaderType, params_cast->mName, params_cast->mEntryPoint);

		auto shader = raw.SpawnOnGPU(mManager->GetParent()->GetDevice());
		return new ShaderResource(mManager, shader);
	}

	// Loads resource and adds resulting task to the specified barrier
	TaskId ResourceCache<ShaderResource>::AsyncLoadDeferred(const void* params,
			ThreadPool* pool,
			IResource** output,
			const TaskBarrierCallback& callback) {
		auto params_cast = reinterpret_cast<const LoadParams<ShaderResource>*>(params);

		LoadParams<ShaderResource> paramsCopy = *params_cast;
		ShaderPreprocessorConfig overrides;
		EmbeddedFileLoader* fileLoader = mManager->GetEmbededFileLoader();
		ShaderLoader* shaderLoader = &mLoader;
		if (params_cast->mOverrides)
			overrides = *params_cast->mOverrides;

		auto resource = new ShaderResource(mManager, nullptr);
		DG::IRenderDevice* renderDevice = mManager->GetParent()->GetDevice();
		TaskBarrier* barrier = resource->GetLoadBarrier();
		barrier->SetCallback(callback);

		TaskId preprocessShader;

		{
			auto queue = pool->GetQueue();
			
			PipeId pipe = queue.MakePipe();
			preprocessShader = queue.MakeTask([pipe, paramsCopy, overrides, fileLoader, shaderLoader](const TaskParams& params) {
				ShaderPreprocessorOutput output;
				shaderLoader->Load(paramsCopy.mSource, fileLoader, &output, &overrides);
				params.mPool->WritePipe(pipe, new RawShader(output, paramsCopy.mShaderType, paramsCopy.mName, paramsCopy.mEntryPoint));
			});

			TaskId createShader = queue.MakeTask([pipe, renderDevice, resource](const TaskParams& params) {
				auto& value = params.mPool->ReadPipe(pipe);

				RawShader* shaderRaw = value.cast<RawShader*>();
				DG::IShader* shader = shaderRaw->SpawnOnGPU(renderDevice);
				delete shaderRaw;

				resource->SetShader(shader);
			}, barrier, ASSIGN_THREAD_MAIN);

			queue.Dependencies(pipe).After(preprocessShader);
			queue.Dependencies(createShader).After(pipe);
		}

		*output = resource;
		
		return preprocessShader;
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