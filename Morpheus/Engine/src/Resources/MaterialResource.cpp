#include <Engine/Resources/MaterialResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Engine.hpp>
#include <Engine/ThreadTasks.hpp>

#include <fstream>

namespace Morpheus {

	MaterialResource::MaterialResource(ResourceManager* manager) :
		IResource(manager),
		mPipeline(nullptr),
		bSourced(false) {
	}

	MaterialResource::MaterialResource(ResourceManager* manager,
		PipelineResource* pipeline,
		const std::vector<TextureResource*>& textures,
		const std::vector<DG::IBuffer*>& buffers,
		const apply_material_func_t& applyFunc) :
		IResource(manager),
		bSourced(false) {
		Initialize(pipeline, textures, buffers, applyFunc);
	}

	void MaterialResource::Initialize(PipelineResource* pipeline,
		const std::vector<TextureResource*>& textures,
		const std::vector<DG::IBuffer*>& buffers,
		const apply_material_func_t& applyFunc) {

		pipeline->AddRef();
		for (auto tex : textures) {
			tex->AddRef();
		}

		for (auto buf : mUniformBuffers) {
			buf->Release();
		}

		if (mPipeline)
			mPipeline->Release();

		for (auto item : mTextures) {
			item->Release();
		}

		mUniformBuffers = buffers;
		mPipeline = pipeline;
		mTextures = textures;
		mApplyFunc = applyFunc;
	}

	void MaterialResource::SetSource(const std::unordered_map<std::string, MaterialResource*>::iterator& it) {
		mSourceIterator = it;
		bSourced = true;
	}

	MaterialResource::~MaterialResource() {
		if (mPipeline)
			mPipeline->Release();
		for (auto item : mTextures) {
			item->Release();
		}
		for (auto buf : mUniformBuffers) {
			buf->Release();
		}
	}

	MaterialResource* MaterialResource::ToMaterial() {
		return this;
	}

	void MaterialLoader::Load(ResourceManager* manager,
		const std::string& source, 
		const MaterialFactory& prototypeFactory, 
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

		std::string prototype_str;
		json["Prototype"].get_to(prototype_str);

		prototypeFactory.Spawn(prototype_str, manager, source, path, json, loadinto);
	}

	TaskId MaterialLoader::AsyncLoad(ResourceManager* manager,
		const std::string& source,
		const MaterialFactory& prototypeFactory,
		ThreadPool* pool,
		TaskBarrierCallback barrierCallback,
		MaterialResource* loadInto) {

		loadInto->mLoadBarrier.SetCallback(barrierCallback);

		auto queue = pool->GetQueue();
		PipeId filePipe;
		TaskId readFile = ReadFileToMemoryJobDeferred(source, &queue, &filePipe);
		TaskId spawnPrototype = queue.MakeTask(
			[filePipe, prototypeFactory, manager, 
			source, pool, loadInto](const TaskParams& params) {

			nlohmann::json json;

			{
				auto& value = params.mPool->ReadPipe(filePipe);
				std::unique_ptr<ReadFileToMemoryResult> 
					contents(value.cast<ReadFileToMemoryResult*>());
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
			prototypeFactory.SpawnAsync(
				prototype_str, manager, source,
				path, json, pool, loadInto);
 		}, &loadInto->mLoadBarrier);

		queue.Dependencies(spawnPrototype).After(filePipe);
		return readFile;
	}

	ResourceCache<MaterialResource>::ResourceCache(ResourceManager* manager) : 
		mManager(manager) {
	}

	ResourceCache<MaterialResource>::~ResourceCache() {
		Clear();
	}

	IResource* ResourceCache<MaterialResource>::Load(const void* params) {
		auto params_cast = reinterpret_cast<
			const LoadParams<MaterialResource>*>(params);
		auto src = params_cast->mSource;
		
		{
			std::shared_lock<std::shared_mutex> lock(mResourceMapMutex);
			auto it = mResourceMap.find(src);
			if (it != mResourceMap.end()) {
				return it->second;
			}
		}

		MaterialResource* resource = new MaterialResource(mManager);
		MaterialLoader::Load(mManager, src, mPrototypeFactory, resource);

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
		
		auto params_cast = reinterpret_cast<
			const LoadParams<MaterialResource>*>(params);
		auto src = params_cast->mSource;

		{
			std::shared_lock<std::shared_mutex> lock(mResourceMapMutex);
			auto it = mResourceMap.find(src);
			if (it != mResourceMap.end()) {
				*output = it->second;
				return TASK_NONE;
			}
		}

		MaterialResource* resource = new MaterialResource(mManager);
		TaskId task = MaterialLoader::AsyncLoad(mManager, src, 
			mPrototypeFactory, threadPool, callback, resource);

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

		mViewRegistry.clear();
	}
}