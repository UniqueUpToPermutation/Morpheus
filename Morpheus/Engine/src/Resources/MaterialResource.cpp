#include <Engine/Resources/MaterialResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Engine.hpp>
#include <Engine/ThreadTasks.hpp>

#include <fstream>

namespace Morpheus {

	MaterialResource::MaterialResource(ResourceManager* manager,
		ResourceCache<MaterialResource>* cache) :
		IResource(manager),
		mResourceBinding(nullptr),
		mPipeline(nullptr),
		mCache(cache),
		bSourced(false) {
		mEntity = cache->mViewRegistry.create();
	}

	MaterialResource::MaterialResource(ResourceManager* manager,
		DG::IShaderResourceBinding* binding, 
		PipelineResource* pipeline,
		const std::vector<TextureResource*>& textures,
		const std::vector<DG::IBuffer*>& buffers,
		ResourceCache<MaterialResource>* cache) :
		IResource(manager),
		mCache(cache),
		bSourced(false) {
		mEntity = cache->mViewRegistry.create();
		Init(binding, pipeline, textures, buffers);
	}

	void MaterialResource::Init(DG::IShaderResourceBinding* binding, 
		PipelineResource* pipeline,
		const std::vector<TextureResource*>& textures,
		const std::vector<DG::IBuffer*>& buffers) {

		pipeline->AddRef();
		for (auto tex : textures) {
			tex->AddRef();
		}

		for (auto buf : mUniformBuffers) {
			buf->Release();
		}

		if (mResourceBinding)
			mResourceBinding->Release();
		if (mPipeline)
			mPipeline->Release();

		for (auto item : mTextures) {
			item->Release();
		}

		mUniformBuffers = buffers;
		mResourceBinding = binding;
		mPipeline = pipeline;
		mTextures = textures;
	}

	void MaterialResource::SetSource(const std::unordered_map<std::string, MaterialResource*>::iterator& it) {
		mSourceIterator = it;
		bSourced = true;
	}

	MaterialResource::~MaterialResource() {
		if (mResourceBinding)
			mResourceBinding->Release();
		if (mPipeline)
			mPipeline->Release();
		for (auto item : mTextures) {
			item->Release();
		}
		for (auto buf : mUniformBuffers) {
			buf->Release();
		}
		mCache->mViewRegistry.destroy(mEntity);
	}

	MaterialResource* MaterialResource::ToMaterial() {
		return this;
	}

	MaterialLoader::MaterialLoader(ResourceManager* manager,
		ResourceCache<MaterialResource>* cache) :
		mManager(manager),
		mCache(cache) {
	}

	void MaterialLoader::Load(const std::string& source, 
		const MaterialPrototypeFactory& prototypeFactory, 
		MaterialResource* loadinto) {
		std::cout << "Loading " << source << "..." << std::endl;

		std::ifstream stream;
		stream.exceptions(std::ios::failbit | std::ios::badbit);
		stream.open(source);

		nlohmann::json json;
		stream >> json;

		stream.close();

		std::string path = ".";
		auto path_cutoff = source.rfind('/');
		if (path_cutoff != std::string::npos) {
			path = source.substr(0, path_cutoff);
		}

		Load(json, source, path, prototypeFactory, loadinto);
	}

	void MaterialLoader::Load(const nlohmann::json& json, 
		const std::string& source, const std::string& path,
		const MaterialPrototypeFactory& prototypeFactory,
		MaterialResource* loadinto) {

		std::string prototype_str;
		json["Prototype"].get_to(prototype_str);
	
		std::unique_ptr<MaterialPrototype> materialPrototype(
			prototypeFactory.Spawn(prototype_str, mManager, source, path, json));

		if (materialPrototype == nullptr) {
			throw std::runtime_error("Could not find prototype!");
		}

		// Use the material prototype to initialize material
		materialPrototype->InitializeMaterial(mManager->GetParent()->GetDevice(), loadinto);
	}

	TaskId MaterialLoader::AsyncLoad(const std::string& source,
		const MaterialPrototypeFactory& prototypeFactory,
		ThreadPool* pool,
		TaskBarrierCallback barrierCallback,
		MaterialResource* loadInto) {

		loadInto->mLoadBarrier.SetCallback(barrierCallback);

		auto manager = mManager;
		auto device = mManager->GetParent()->GetDevice();
		auto queue = pool->GetQueue();
		PipeId filePipe;
		TaskId readFile = ReadFileToMemoryJobDeferred(source, &queue, &filePipe);
		TaskId spawnPrototype = queue.MakeTask(
			[filePipe, prototypeFactory, manager, 
			source, pool, loadInto, device](const TaskParams& params) {

			nlohmann::json json;

			{
				auto& value = params.mPool->ReadPipe(filePipe);
				std::unique_ptr<ReadFileToMemoryResult> contents(value.cast<ReadFileToMemoryResult*>());
				json = nlohmann::json::parse(contents->GetData());
			}

			// Create prototype
			std::string prototype_str;
			json["Prototype"].get_to(prototype_str);

			std::string path = ".";
			auto path_cutoff = source.rfind('/');
			if (path_cutoff != std::string::npos) {
				path = source.substr(0, path_cutoff);
			}

			MaterialPrototype* prototype;
			TaskId loadOtherResourcesTask = prototypeFactory.SpawnAsyncDeferred(prototype_str, manager, source,
				path, json, pool, &prototype);

			if (prototype == nullptr) {
				throw std::runtime_error("Could not find prototype!");
			}

			{
				auto queue = params.mPool->GetQueue();
				TaskId initMaterial = queue.MakeTask([prototype, device, loadInto](const TaskParams& params) {
					prototype->InitializeMaterial(device, loadInto);
				}, loadInto->GetLoadBarrier(), ASSIGN_THREAD_MAIN);

				// Load resources before we create the material itself
				prototype->ScheduleLoadBefore(queue.Dependencies(initMaterial));
				queue.Schedule(initMaterial);
			}
 		});

		queue.Dependencies(spawnPrototype).After(filePipe);
		return readFile;
	}

	ResourceCache<MaterialResource>::ResourceCache(ResourceManager* manager) : 
		mManager(manager), 
		mLoader(manager, this) {
	}

	ResourceCache<MaterialResource>::~ResourceCache() {
		Clear();
	}

	IResource* ResourceCache<MaterialResource>::Load(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<MaterialResource>*>(params);
		auto src = params_cast->mSource;
		
		{
			std::shared_lock<std::shared_mutex> lock(mResourceMapMutex);
			auto it = mResourceMap.find(src);
			if (it != mResourceMap.end()) {
				return it->second;
			}
		}

		MaterialResource* resource = new MaterialResource(mManager, this);
		mLoader.Load(src, mPrototypeFactory, resource);

		{
			std::unique_lock<std::shared_mutex> lock(mResourceMapMutex);
			resource->SetSource(mResourceMap.emplace(src, resource).first);
		}

		return resource;
	}

	TaskId ResourceCache<MaterialResource>::AsyncLoadDeferred(const void* params,
		ThreadPool* threadPool,
		IResource** output,
		const TaskBarrierCallback& callback) {
		
		auto params_cast = reinterpret_cast<const LoadParams<MaterialResource>*>(params);
		auto src = params_cast->mSource;

		{
			std::shared_lock<std::shared_mutex> lock(mResourceMapMutex);
			auto it = mResourceMap.find(src);
			if (it != mResourceMap.end()) {
				*output = it->second;
				return TASK_NONE;
			}
		}

		MaterialResource* resource = new MaterialResource(mManager, this);
		TaskId task = mLoader.AsyncLoad(src, mPrototypeFactory, threadPool, callback, resource);

		{
			std::unique_lock<std::shared_mutex> lock(mResourceMapMutex);
			resource->SetSource(mResourceMap.emplace(src, resource).first);
		}

		*output = resource;

		return task;
	}

	void ResourceCache<MaterialResource>::Add(IResource* resource, const void* params) {
		throw std::runtime_error("Not implemented!");
	}

	void ResourceCache<MaterialResource>::Unload(IResource* resource) {
		std::unique_lock<std::shared_mutex> lock(mResourceMapMutex);
		auto mat = resource->ToMaterial();

		if (mat->bSourced)
			mResourceMap.erase(mat->mSourceIterator);

		delete resource;
	}

	void ResourceCache<MaterialResource>::Clear() {
		std::unique_lock<std::shared_mutex> lock(mResourceMapMutex);
		for (auto& item : mResourceMap) {
			item.second->ResetRefCount();
			delete item.second;
		}
	}
}