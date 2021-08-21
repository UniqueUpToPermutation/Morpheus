#pragma once

#include <unordered_map>
#include <string>

#include <Engine/Resources/Resource.hpp>
#include <Engine/Frame.hpp>
#include <Engine/Renderer.hpp>

namespace Morpheus {

	typedef int ResourceId;

	constexpr ResourceId InvalidResourceId = -1;

	class IAbstractSerializableComponentType {
	public:
		virtual entt::meta_type GetType() const = 0;
		virtual void Serialize(entt::registry* registry, 
			std::ostream& stream,
			const FrameHeader& header,
			const SerializationSet& subset) const = 0;
		virtual void Deserialize(entt::registry* registry, 
			std::istream& stream,
			const FrameHeader& header) const = 0;
		virtual void BuildResourceTable(entt::registry* registry,
			FrameHeader& header,
			const SerializationSet& subset) const = 0;
	};

	template <typename T>
	class ConcreteSerializableComponentType : 
		public IAbstractSerializableComponentType {
	public:
		entt::meta_type GetType() const override {
			return entt::resolve<T>();
		}
		void Serialize(entt::registry* registry, 
			std::ostream& stream,
			const FrameHeader& header,
			const SerializationSet& subset) const override {
			T::Serialize(registry, stream, header, set);
		}
		void Deserialize(entt::registry* registry, 
			std::istream& stream,
			const FrameHeader& header) const override {
			T::Deserialize(registry, stream, header);
		}
		void BuildResourceTable(entt::registry* registry,
			FrameHeader& header,
			const SerializationSet& subset) const override {
			T::BuildResourceTable(registry, header, subset);
		}
	};

	typedef std::shared_ptr<IAbstractSerializableComponentType> SerializableComponentType;

	template <typename T>
	inline SerializableComponentType MakeSerializableComponentType() {
		return SerializableComponentType(new ConcreteSerializableComponentType<T>());
	}

	struct ArchiveBlobPointer {
		std::streamsize mBegin;
		std::streamsize mSize;
	};
	
	enum class ResourceTableEntryType {
		INTERNAL,
		EXTERNAL
	};

	struct ResourceTableEntry {
		Handle<IResource> mResource;
		ResourceId mId;
		ResourceTableEntryType mType;
		ArchiveBlobPointer mInternalPointer;
	};

	struct SubFrameResourceTableEntry {
		FrameId mFrameId;
		std::string mPathString;
	};
	
	class FrameHeader {
	private:
		FrameId mFrameId = InvalidFrameId;

		std::set<Handle<IResource>> mResourceHandles;
		std::vector<ResourceTableEntry> mResourceEntries;

		// Data that is serialized
		std::unordered_map<std::string,
			ArchiveBlobPointer> mComponentDirectory;
		std::unordered_map<FrameId, SubFrameResourceTableEntry> mSubFrames;
		std::unordered_map<Handle<IResource>, ResourceId, 
			Handle<IResource>::Hasher> mIdsByResource;
		std::unordered_map<MaterialId, MaterialDesc> mMaterialsById;

		std::filesystem::path mPath;
		uint mVersion = 0;

	public:
		inline FrameHeader() {
		}

		inline FrameHeader(const std::filesystem::path& path) :
			mPath(path) {
		}

		inline FrameHeader(Frame* frame) {
			mFrameId = frame->GetFrameId();
			mPath = frame->GetPath();
		}

		void AddSubFrame(Handle<Frame> frame);
		void AddMaterial(Material mat);
		void Add(Handle<IResource> resource);
		ResourceId GetResourceId(Handle<IResource> resource);

		template <typename T>
		Handle<T> GetResourceById(ResourceId id) {
			auto it = mResourcesById.find(id);

			if (it == mResourcesById.end())
				return nullptr;
			
			return dynamic_cast<T*>(it->second.mResource);
		}

		bool FindComponent(const std::string& component, ArchiveBlobPointer* out);
		bool FindComponent(const SerializableComponentType& type, ArchiveBlobPointer* out);

		void SetInternalResourceLocation(ResourceId id, const ArchiveBlobPointer& loc);
		ArchiveBlobPointer GetInternalResourceLocation(ResourceId id);

		void WriteResourceContent(const std::filesystem::path& workingPath, 
			std::ostream& stream);

		void Write(std::ostream& stream);
		static FrameHeader Read(std::istream& stream);

		friend class FrameIO;
	};

	class FrameIO {
	public:
		static void Save(Frame* frame,
			const std::filesystem::path& workingPath,
			std::ostream& stream, 
			const std::vector<SerializableComponentType>& componentTypes);
		static Task Load(Frame* frame, std::istream& stream, 
			const IResourceCacheCollection* resourceCaches,
			const std::vector<SerializableComponentType>& componentTypes);
	};
}