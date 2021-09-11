#pragma once

#include <unordered_map>
#include <string>

#include <Engine/Resources/Resource.hpp>
#include <Engine/Frame.hpp>
#include <Engine/Reflection.hpp>

namespace Morpheus {

	std::unique_ptr<cereal::PortableBinaryInputArchive> 
		MakePortableBinaryInputArchive(std::istream& stream);
	std::unique_ptr<cereal::PortableBinaryOutputArchive>
		MakePortableBinaryOutputArchive(std::ostream& stream);

	template <typename T, typename componentT, bool b_is_resource>
	struct SerializeRedirect {
		static void Do(
			componentT& obj,
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryOutputArchive& arr, 
			IDependencyResolver* dependencies);
	};

	template <typename T, typename componentT, bool b_is_resource>
	struct DeserializeRedirect {
		static componentT Do(
			ResourceCache* cache,
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& arr, 
			IDependencyResolver* dependencies,
			Handle<IResource>* resourceOut);
	};

	template <typename T, typename componentT>
	struct DeserializeRedirect<T, componentT, true> {
		static componentT Do(
			ResourceCache* cache,
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& arr, 
			IDependencyResolver* dependencies,
			Handle<IResource>* resourceOut) {

			T res;
			res.BinaryDeserializeReference(workingPath, arr);

			Handle<T> handle;

			if (cache) {
				handle = cache->FindOrEmplace<T>(res.GetUniversalId(), std::move(res)).template TryCast<T>();
			} else {
				handle = Handle<T>(std::move(res));
			}

			*resourceOut = handle.template DownCast<IResource>();
			return componentT(handle);
		}
	};

	template <typename T, typename componentT>
	struct DeserializeRedirect<T, componentT, false> {
		static componentT Do(
			ResourceCache* cache,
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& arr, 
			IDependencyResolver* dependencies,
			Handle<IResource>* resourceOut) {

			return T::Deserialize(arr, dependencies);
		}
	};

	template <typename T, typename componentT>
	struct SerializeRedirect<T, componentT, true> {
		static void Do(
			componentT& obj,
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryOutputArchive& arr,
			IDependencyResolver* dependencies) {

			obj.BinarySerializeReference(workingPath, arr);
		}
	};

	template <typename T, typename componentT>
	struct SerializeRedirect<T, componentT, false> {
		static void Do(
			componentT& obj,
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryOutputArchive& arr,
			IDependencyResolver* dependencies) {

			T::Serialize(obj, arr, dependencies);
		}
	};

	void WriteInt64(cereal::PortableBinaryOutputArchive& arr, size_t sz);
	void WriteEntity(cereal::PortableBinaryOutputArchive& arr, entt::entity e);
	void ReadInt64(cereal::PortableBinaryInputArchive& arr, size_t& sz);
	void ReadEntity(cereal::PortableBinaryInputArchive& arr, entt::entity& e);

	template <typename objectT, 
		typename componentT,
		bool b_is_resource,
		bool b_is_component,
		bool b_load_with_frame,
		int priority = 0>
	class SerializableTypeImpl final : 
		public IAbstractSerializableType {
	public:
		entt::meta_type GetType() const override {
			return entt::resolve<objectT>();
		}

		// Serializable all components of the given type
		void Serialize(
			const std::filesystem::path& workingPath,
			entt::registry& registry, 
			std::ostream& output,
			IDependencyResolver* dependencies) const override {

			auto serializer = MakePortableBinaryOutputArchive(output);

			auto view = registry.view<componentT>();
			size_t sz = view.size();

			WriteInt64(*serializer, sz);			

			for (auto e : view) {
				WriteEntity(*serializer, e);
				auto& component = view.template get<componentT>(e);
				SerializeRedirect<objectT, componentT, b_is_resource>::Do(
					component, workingPath, *serializer, dependencies);
			}
		}

		// Deserialize / initialize all components of the given type
		void Deserialize(
			ResourceCache* cache,
			const std::filesystem::path& workingPath,
			entt::registry& registry,
			std::istream& input,
			IDependencyResolver* dependencies,
			std::unordered_map<entt::entity, Handle<IResource>>* resources) const override {

			auto deserializer = MakePortableBinaryInputArchive(input);
			
			auto view = registry.view<componentT>();

			size_t sz;

			ReadInt64(*deserializer, sz);

			for (size_t i = 0; i < sz; ++i) {
				entt::entity e;
				ReadEntity(*deserializer, e);

				if (!registry.valid(e)) {
					auto result = registry.create(e);
					if (result != e) {
						throw std::runtime_error("Could not create valid entity!");
					}
				}

				Handle<IResource> resourceHandle;

				componentT comp = DeserializeRedirect<objectT, componentT, b_is_resource>::Do(
					cache, workingPath, *deserializer, dependencies, &resourceHandle);
				
				registry.emplace<componentT>(e, std::move(comp));

				if (resources) {
					resources->emplace(e, resourceHandle);
				}
			}
		}

		bool IsResource() const override {
			return b_is_resource;
		}

		bool IsComponent() const override {
			return b_is_component;
		}

		bool IsAutoLoadResource() const override {
			return b_load_with_frame;
		}

		int GetLoadPriority() const override {
			return priority;
		}
	};

	template <typename T>
	using SerializableComponentType = 
		SerializableTypeImpl<T, T, false, true, true>;

	template <typename T>
	using SerializableResourceType = 
		SerializableTypeImpl<T, ResourceComponent<T>, true, false, false>;

	// Materials are always loaded completely
	using MaterialSerializableType = 
		SerializableTypeImpl<Material, ResourceComponent<Material>, true, false, true, -1>;

	typedef std::shared_ptr<IAbstractSerializableType> SerializableType;

	template <typename T>
	inline SerializableType MakeSerializableComponentType() {
		return SerializableType(new SerializableComponentType<T>());
	}

	template <typename T>
	inline SerializableType MakeSerializableResourceType() {
		return SerializableType(new SerializableResourceType<T>());
	}

	template <>
	SerializableType MakeSerializableComponentType<Material>();

	SerializableType GetSerializableType(const entt::meta_type& type);
	void ForEachSerializableType(std::function<void(const SerializableType)> func);
	SerializableType AddSerializableTypes(SerializableType type);

	class FrameTable;

	enum class DependencyEntryType {
		INTERNAL,
		EXTERNAL_FRAME_RESOURCE,
		EXTERNAL_FRAME,
		EXTERNAL
	};

	struct FrameDependency {
		DependencyEntryType mType;
		Handle<IResource> mResource;
		std::string mTypeName;
		UniversalIdentifier mIdentifier;
		ArchiveBlobPointer mBlob;

		template <typename Archive>
		void serialize(Archive& ar);
	};

	class FrameTable : public IDependencyResolver {
	private:
		std::unordered_map<entt::entity, ArchiveBlobPointer> mInternalResourceTable;
		std::unordered_map<std::string, ArchiveBlobPointer> mTypeDirectory;
		std::unordered_map<std::string, entt::entity> mNameToEntity;
		std::unordered_map<ResourceId, FrameDependency> mDependencies;
		std::unordered_map<Handle<IResource>, 
			ResourceId, Handle<IResource>::Hasher> mPointerToId;
		ResourceId mCurrentId = 0;
		Handle<Frame> mFrame;

		entt::registry mExternalResourceRegistry;

	public:
		inline const auto& InternalResourceTable() const {
			return mInternalResourceTable;
		}

		inline const auto& ComponentDirectory() const {
			return mTypeDirectory;
		}

		inline const auto& NameToEntity() const {
			return mNameToEntity;
		}

		inline FrameTable(Handle<Frame> frame) : mFrame(frame) {
		}

		bool FindComponent(const std::string& component, ArchiveBlobPointer* out);
		bool FindComponent(const SerializableType& type, ArchiveBlobPointer* out);

		void WriteResourceData(std::ostream& stream);
		void WriteComponents(
			const std::filesystem::path& workingPath,
			std::ostream& stream, 
			const std::vector<SerializableType>& types);
		void WriteResourceComponents(
			const std::filesystem::path& workingPath,
			std::ostream& stream, 
			const std::vector<SerializableType>& types);

		void ReadResourceData(std::istream& stream);
		void ReadResourceComponents(
			ResourceCache& cache,
			std::istream& stream,
			const std::filesystem::path& workingPath,
			const std::vector<SerializableType>& types);
		void ReadComponents(std::istream& stream,
			const std::filesystem::path& workingPath,
			const std::vector<SerializableType>& types);
		void ReadFramesRecursive(
			ResourceCache& cache,
			const std::vector<SerializableType>& types);

		ResourceId AddDependency(Handle<IResource> resource) override;
		Handle<IResource> GetDependency(ResourceId id) const override;

		void Read(std::istream& stream);
		void FindAndThenRead(std::istream& stream);
		void Write(std::ostream& stream);

		friend class FrameIO;
	};

	class FrameIO {
	public:
		static void Save(Frame& frame,
			std::ostream& stream, 
			const std::vector<SerializableType>& componentTypes);
		static void Save(Frame& frame,
			const std::filesystem::path& path,
			const std::vector<SerializableType>& componentTypes);
		static Frame Load(std::istream& stream,
			const std::filesystem::path& workingPath,
			ResourceCache& cache,
			const std::vector<SerializableType>& componentTypes);
		static Frame Load(const std::filesystem::path& path,
			ResourceCache& cache,
			const std::vector<SerializableType>& componentTypes);
	};
}