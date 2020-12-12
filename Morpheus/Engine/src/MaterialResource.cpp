#include <Engine/MaterialResource.hpp>
#include <Engine/ResourceManager.hpp>
#include <Engine/PipelineResource.hpp>
#include <Engine/TextureResource.hpp>

#include <fstream>

namespace Morpheus {

	MaterialResource::MaterialResource(ResourceManager* manager,
		ResourceCache<MaterialResource>* cache) :
		Resource(manager),
		mResourceBinding(nullptr),
		mPipeline(nullptr),
		mCache(cache) {
		mEntity = cache->mViewRegistry.create();
	}

	MaterialResource::MaterialResource(ResourceManager* manager,
		DG::IShaderResourceBinding* binding, 
		PipelineResource* pipeline,
		const std::vector<TextureResource*>& textures,
		const std::vector<DG::IBuffer*>& buffers,
		const std::string& source,
		ResourceCache<MaterialResource>* cache) : 
		Resource(manager),
		mCache(cache) {
		mEntity = cache->mViewRegistry.create();
		Init(binding, pipeline, textures, buffers, source);
	}

	void MaterialResource::Init(DG::IShaderResourceBinding* binding, 
		PipelineResource* pipeline,
		const std::vector<TextureResource*>& textures,
		const std::vector<DG::IBuffer*>& buffers,
		const std::string& source) {

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
		mSource = source;
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

		// Use a prototype to load the material
		if (json.contains("Prototype")) {

			std::string prototype_str;
			json["Prototype"].get_to(prototype_str);
		
			std::unique_ptr<MaterialPrototype> materialPrototype(
				prototypeFactory.Spawn(prototype_str, mManager, source, path, json));

			if (materialPrototype == nullptr) {
				throw std::runtime_error("Could not find prototype!");
			}

			// Use the material prototype to initialize material
			materialPrototype->InitializeMaterial(mManager, mCache, loadinto);

		} else {
			// Fall back to a default loader for the material
			std::string pipeline_str;
			json["Pipeline"].get_to(pipeline_str);

			auto pipeline = mManager->Load<PipelineResource>(pipeline_str);

			DG::IShaderResourceBinding* binding = nullptr;
			pipeline->GetState()->CreateShaderResourceBinding(&binding, true);

			std::vector<TextureResource*> textures;
			if (json.contains("Textures")) {
				for (const auto& item : json["Textures"]) {

					std::string binding_loc;
					item["Binding"].get_to(binding_loc);

					DG::SHADER_TYPE shader_type = ReadShaderType(item["ShaderType"]);

					std::string source;
					item["Source"].get_to(source);

					auto texture = mManager->Load<TextureResource>(source);
					textures.emplace_back(texture);

					auto variable = binding->GetVariableByName(shader_type, binding_loc.c_str());
					if (variable) {
						variable->Set(texture->GetShaderView());
					} else {
						std::cout << "Warning could not find binding " << binding_loc << "!" << std::endl;
					}
				}
			}

			std::vector<DG::IBuffer*> buffers;
			loadinto->Init(binding, pipeline, textures, buffers, source);

			for (auto tex : textures) {
				tex->Release();
			}

			pipeline->Release();
		}
	}

	ResourceCache<MaterialResource>::ResourceCache(ResourceManager* manager) : 
		mManager(manager), 
		mLoader(manager, this) {
	}

	ResourceCache<MaterialResource>::~ResourceCache() {
		Clear();
	}

	Resource* ResourceCache<MaterialResource>::Load(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<MaterialResource>*>(params);
		auto src = params_cast->mSource;

		auto it = mResources.find(src);
		if (it != mResources.end()) {
			return it->second;
		}

		MaterialResource* resource = new MaterialResource(mManager, this);
		mLoader.Load(src, mPrototypeFactory, resource);
		mResources[src] = resource;
		return resource;
	}

	Resource* ResourceCache<MaterialResource>::DeferredLoad(const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<MaterialResource>*>(params);
		auto src = params_cast->mSource;

		auto it = mResources.find(src);
		if (it != mResources.end()) {
			return it->second;
		}

		MaterialResource* resource = new MaterialResource(mManager, this);
		mResources[src] = resource;
		mDeferredResources.emplace_back(std::make_pair(resource, *params_cast));
		return resource;
	}

	void ResourceCache<MaterialResource>::ProcessDeferred() {
		for (auto resource : mDeferredResources) {
			mLoader.Load(resource.second.mSource, mPrototypeFactory, resource.first);
		}

		mDeferredResources.clear();
	}

	void ResourceCache<MaterialResource>::Add(Resource* resource, const void* params) {
		auto params_cast = reinterpret_cast<const LoadParams<MaterialResource>*>(params);
		auto src = params_cast->mSource;

		auto pipeline = resource->ToMaterial();

		auto it = mResources.find(src);
		if (it != mResources.end()) {
			if (it->second != pipeline)
				Unload(it->second);
			else
				return;
		}
		mResources[src] = pipeline;
	}

	void ResourceCache<MaterialResource>::Unload(Resource* resource) {
		auto mat = resource->ToMaterial();

		auto it = mResources.find(mat->GetSource());
		if (it != mResources.end()) {
			if (it->second == mat) {
				mResources.erase(it);
			}
		}

		delete resource;
	}

	void ResourceCache<MaterialResource>::Clear() {
		for (auto& item : mResources) {
			item.second->ResetRefCount();
			delete item.second;
		}
	}
}