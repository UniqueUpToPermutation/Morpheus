#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Engine.hpp>
#include <Engine/Geometry.hpp>
#include <unordered_map>

namespace DG = Diligent;

namespace Assimp {
	class Importer;
}

struct aiScene;

namespace Morpheus {

	class GeometryResource;
	class RawGeometry {
	private:
		DG::BufferDesc mVertexBufferDesc;
		DG::BufferDesc mIndexBufferDesc;

		DG::DrawAttribs mUnindexedDrawAttribs;
		DG::DrawIndexedAttribs mIndexedDrawAttribs;

		std::vector<uint8_t> mVertexBufferData;
		std::vector<uint8_t> mIndexBufferData;

		PipelineResource* mPipeline = nullptr;
		BoundingBox mAabb;
		bool bHasIndexBuffer;

	public:
		inline RawGeometry() {
		}
		RawGeometry(const RawGeometry& other) = delete;
		RawGeometry(RawGeometry&& other);
		~RawGeometry();

		void Set(PipelineResource* pipeline,
			const DG::BufferDesc& vertexBufferDesc, 
			std::vector<uint8_t>&& vertexBufferData,
			const DG::DrawAttribs& unindexedDrawAttribs,
			const BoundingBox& aabb);

		void Set(PipelineResource* pipeline,
			const DG::BufferDesc& vertexBufferDesc,
			const DG::BufferDesc& indexBufferDesc,
			std::vector<uint8_t>&& vertexBufferData,
			std::vector<uint8_t>&& indexBufferData,
			DG::DrawIndexedAttribs& indexedDrawAttribs,
			const BoundingBox& aabb);

		inline RawGeometry(PipelineResource* pipeline,
		const DG::BufferDesc& vertexBufferDesc, 
			std::vector<uint8_t>&& vertexBufferData,
			const DG::DrawAttribs& unindexedDrawAttribs,
			const BoundingBox& aabb) {
			Set(pipeline,
				vertexBufferDesc, std::move(vertexBufferData), 
				unindexedDrawAttribs, aabb);
		}

		inline RawGeometry(PipelineResource* pipeline,
			const DG::BufferDesc& vertexBufferDesc,
			const DG::BufferDesc& indexBufferDesc,
			std::vector<uint8_t>&& vertexBufferData,
			std::vector<uint8_t>&& indexBufferData,
			DG::DrawIndexedAttribs& indexedDrawAttribs,
			const BoundingBox& aabb);

		void SpawnOnGPU(DG::IRenderDevice* device, 
			DG::IBuffer** vertexBufferOut, 
			DG::IBuffer** indexBufferOut);

		void SpawnOnGPU(DG::IRenderDevice* device,
			GeometryResource* writeTo);
	};

	class GeometryResource : public IResource {
	private:
		DG::IBuffer* mVertexBuffer;
		DG::IBuffer* mIndexBuffer;

		uint mVertexBufferOffset;

		PipelineResource* mPipeline;

		DG::DrawIndexedAttribs mIndexedAttribs;
		DG::DrawAttribs mUnindexedAttribs;

		BoundingBox mBoundingBox;

		std::unordered_map<std::string, GeometryResource*>::iterator mIterator;

		void InitIndexed(DG::IBuffer* vertexBuffer, DG::IBuffer* indexBuffer,
			uint vertexBufferOffset, 
			const DG::DrawIndexedAttribs& attribs,
			PipelineResource* pipeline, 
			const BoundingBox& aabb);

		void Init(DG::IBuffer* vertexBuffer,
			uint vertexBufferOffset,
			const DG::DrawAttribs& attribs,
			PipelineResource* pipeline, 
			const BoundingBox& aabb);

	public:

		GeometryResource(ResourceManager* manager);
		~GeometryResource();

		inline bool IsReady() const {
			return mVertexBuffer != nullptr;
		}

		inline DG::IBuffer* GetVertexBuffer() {
			return mVertexBuffer;
		}

		inline DG::IBuffer* GetIndexBuffer() {
			return mIndexBuffer;
		}

		inline uint GetVertexBufferOffset() const {
			return mVertexBufferOffset;
		}

		inline PipelineResource* GetPipeline() {
			return mPipeline;
		}

		inline DG::DrawIndexedAttribs GetIndexedDrawAttribs() const {
			return mIndexedAttribs;
		}

		inline DG::DrawAttribs GetDrawAttribs() const {
			return mUnindexedAttribs;
		}

		inline BoundingBox GetBoundingBox() const {
			return mBoundingBox;
		}

		entt::id_type GetType() const override {
			return resource_type::type<GeometryResource>;
		}

		GeometryResource* ToGeometry() override;

		friend class GeometryLoader;
		friend class RawGeometry;
		friend class ResourceCache<GeometryResource>;
	};

	template <>
	struct LoadParams<GeometryResource> {
		// If pipeline resource is nullptr, then default to loading from pipeline source
		PipelineResource* mPipelineResource;
		std::string mPipelineSource;
		std::string mSource;

		static LoadParams<GeometryResource> FromString(const std::string& str) {
			throw std::runtime_error("Cannot create geometry resource load params from string!");
		}
	};

	class GeometryLoader {
	public:
		static void Load(DG::IRenderDevice* device, const std::string& source, 
			PipelineResource* pipeline, GeometryResource* loadinto);

		static void Load(const aiScene* scene,
			PipelineResource* pipeline,
			RawGeometry* geometryOut);

		static TaskId LoadAsync(DG::IRenderDevice* device, 
			ThreadPool* pool,
			const std::string& source,
			const TaskBarrierCallback& callback,
			PipelineResource* pipeline, 
			GeometryResource* loadinto);
	};

	template <>
	class ResourceCache<GeometryResource> : public IResourceCache {
	private:
		std::unordered_map<std::string, GeometryResource*> mResourceMap;
		GeometryLoader mLoader;
		ResourceManager* mManager;

		std::shared_mutex mMutex;

	public:
		ResourceCache(ResourceManager* manager);
		~ResourceCache();

		IResource* Load(const void* params) override;
		TaskId AsyncLoadDeferred(const void* params,
			ThreadPool* threadPool,
			IResource** output,
			const TaskBarrierCallback& callback = nullptr) override;
		void Add(IResource* resource, const void* params) override;
		void Unload(IResource* resource) override;
		void Clear() override;
	};
}