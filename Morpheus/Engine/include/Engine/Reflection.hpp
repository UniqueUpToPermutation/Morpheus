#pragma once

#include <Engine/Entity.hpp>
#include <Engine/Resources/Cache.hpp>
#include <filesystem>

namespace Morpheus {
	// Copyable interface
	class IAbstractCopyableType {
	public:
		virtual entt::meta_type GetType() const = 0;
		virtual void CopyAll(entt::registry& src, entt::registry& dest, 
			const std::unordered_map<entt::entity, entt::entity>& entityMap);
	};

	typedef std::shared_ptr<IAbstractCopyableType> CopyableType;

	template <typename T>
	class CopyableTypeImpl : public IAbstractCopyableType {
	public:
		entt::meta_type GetType() const override {
			return entt::resolve<T>();
		}

		void CopyAll(entt::registry& src, entt::registry& dest, 
			const std::unordered_map<entt::entity, entt::entity>& entityMap) {
			auto tView = src.view<T>();

			for (auto e : tView) {
				auto it = entityMap.find(e);
				if (it != entityMap.end()) {
					auto instance = tView.get(e);
					dest.emplace<T>(it->second, std::get<0>(instance));
				}
			}
		}
	};

	CopyableType GetCopyableType(const entt::meta_type& type);
	CopyableType AddCopyableType(CopyableType type);
	void ForEachCopyableType(std::function<void(const CopyableType)> func);

	template <typename T>
	inline CopyableType MakeCopyableComponentType() {
		return AddCopyableType(CopyableType(new CopyableTypeImpl<T>()));
	}

	template <typename T>
	inline CopyableType GetCopyableType() {
		return GetCopyableType(entt::resolve<T>());
	}


	// Serializable interface
	typedef int ResourceId;

	constexpr ResourceId InvalidResourceId = -1;

	class IAbstractSerializableType {
	public:
		virtual entt::meta_type GetType() const = 0;
		virtual bool IsResource() const = 0;
		virtual bool IsComponent() const = 0;
		virtual bool IsAutoLoadResource() const = 0;
		virtual void Serialize(const std::filesystem::path& workingPath,
			entt::registry& registry, 
			std::ostream& output,
			IDependencyResolver* dependencies) const = 0;
		virtual int GetLoadPriority() const = 0;
		virtual void Deserialize(
			ResourceCache* cache,
			const std::filesystem::path& workingPath,
			entt::registry& registry,
			std::istream& input,
			IDependencyResolver* dependencies,
			std::unordered_map<entt::entity, Handle<IResource>>* resources) const = 0;
	};

	class IDependencyResolver {
	public:
		virtual ResourceId AddDependency(Handle<IResource> resource) = 0;
		virtual Handle<IResource> GetDependency(ResourceId id) const = 0;
	};
}