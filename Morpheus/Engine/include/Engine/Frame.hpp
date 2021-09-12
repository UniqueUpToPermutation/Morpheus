#pragma once

#include <Engine/Entity.hpp>
#include <Engine/Components/Camera.hpp>
#include <Engine/Components/ResourceComponent.hpp>
#include <Engine/Resources/Resource.hpp>

#include <stack>
#include <filesystem>

namespace Morpheus {
	class DepthFirstNodeIterator {
	private:
		std::stack<entt::entity> mNodeStack;
		entt::registry* mRegistry;

	public:
		DepthFirstNodeIterator(entt::registry* registry, 
			entt::entity start) {
			mNodeStack.emplace(start);
			mRegistry = registry;
		}

		inline entt::entity operator()() {
			return mNodeStack.top();
		}

		inline explicit operator bool() const {
			return !mNodeStack.empty();
		}

		inline DepthFirstNodeIterator& operator++() {
			auto& top = mNodeStack.top();

			mNodeStack.emplace(mRegistry->get<HierarchyData>(top).mFirstChild);
			
			while (!mNodeStack.empty() && mNodeStack.top() == entt::null) {
				mNodeStack.pop();
				if (!mNodeStack.empty()) {
					top = mNodeStack.top();
					mNodeStack.pop();
					mNodeStack.emplace(mRegistry->get<HierarchyData>(top).mNext);
				}
			}

			return *this;
		}
	};

	enum class IteratorDirection {
		DOWN,
		UP
	};

	class DepthFirstNodeDoubleIterator {
	private:
		std::stack<entt::entity> mNodeStack;
		IteratorDirection mDirection;
		entt::registry* mRegistry;

	public:
		DepthFirstNodeDoubleIterator(entt::registry* registry, entt::entity start) {
			mNodeStack.emplace(start);
			mDirection = IteratorDirection::DOWN;
			mRegistry = registry;
		}

		inline entt::entity operator()() {
			return mNodeStack.top();
		}

		inline IteratorDirection GetDirection() const {
			return mDirection;
		}

		inline explicit operator bool() const {
			return !mNodeStack.empty();
		}

		inline DepthFirstNodeDoubleIterator& operator++() {
			auto& top = mNodeStack.top();

			if (mDirection == IteratorDirection::UP) {
				mNodeStack.pop();
				mNodeStack.emplace(mRegistry->get<HierarchyData>(top).mNext);
				mDirection = IteratorDirection::DOWN;
			} else {
				mNodeStack.emplace(mRegistry->get<HierarchyData>(top).mFirstChild);
				mDirection = IteratorDirection::DOWN;
			}
			
			if (!mNodeStack.empty() && mNodeStack.top() == entt::null) {
				mNodeStack.pop();
				if (!mNodeStack.empty()) {
					mDirection = IteratorDirection::UP;
				}
			}

			return *this;
		}
	};

	struct SubFrameComponent {
		Handle<Frame> mFrame;
	};

	class Frame : public IFrameAbstract {
	private:
		std::filesystem::path mPath;

		entt::registry mRegistry;
		entt::entity mRoot;
		entt::entity mCamera = entt::null;

		std::unordered_map<entt::entity, ArchiveBlobPointer> mInternalResourceTable;
		std::unordered_map<entt::entity, Handle<IResource>> mEntityToResource;
		std::unordered_map<std::string, entt::entity> mNameToEntity;

	public:
		Frame(Frame&&) = default;
		Frame& operator=(Frame&&) = default;

		Frame(const Frame&) = delete;
		Frame& operator=(const Frame&) = delete;

		const std::unordered_map<entt::entity, ArchiveBlobPointer>&
			GetResourceTable() const override;

		inline auto GetNamesBegin() const {
			return mNameToEntity.cbegin();
		}

		inline auto GetNamesEnd() const {
			return mNameToEntity.cend();
		}

		inline const auto& GetEntityToResources() const {
			return mEntityToResource;
		}

		inline auto GetEntityToResourcesBegin() const {
			return mEntityToResource.cbegin();
		}

		inline auto GetEntityToResourcesEnd() const {
			return mEntityToResource.cend();
		}

		Handle<IResource> GetResourceAbstract(entt::entity e) const override;
		entt::entity GetEntity(const std::string& name) const override;

		template <typename T>
		Handle<T> GetResource(entt::entity e) {
			return mRegistry.try_get<ResourceComponent<T>>(e);
		}

		template <typename T>
		Handle<T> GetResource(const std::string& name) {
			auto entity = GetEntity(name);
			if (entity != entt::null) {
				return GetResource<T>(entity);
			} else {
				return Handle<T>();
			}
		}

		inline void AssignName(const std::string& name, entt::entity e) {
			mNameToEntity[name] = e;
		}

		inline void UnassignName(const std::string& name) {
			mNameToEntity.erase(name);
		}

		inline entt::registry& Registry() {
			return mRegistry;
		}
		inline entt::entity GetRoot() const {
			return mRoot;
		}
		inline entt::entity GetCamera() const {
			return mCamera;
		}
		inline void SetCamera(entt::entity e) {
			mCamera = e;
		}

		template <typename T>
		entt::entity AttachResource(Handle<T> resource) {
			auto e = mRegistry.create();
			mRegistry.emplace<ResourceComponent<T>>(e, resource);

			mEntityToResource[e] = resource;

			resource->mEntity = e;
			resource->mFrame = this;

			return e;
		}

		template <typename T>
		void RemoveResource(Handle<T> resource) {
			mRegistry.remove<ResourceComponent<T>>(resource->mEntity);
			mEntityToResource.erase(resource->mEntity);

			resource->mEntity = entt::null;
			resource->mFrame = nullptr;
		}

		entt::entity SpawnDefaultCamera(Camera** cameraOut = nullptr);
		entt::entity CreateEntity(entt::entity parent);
		void Destroy(entt::entity ent);

		Frame();
		~Frame() = default;
		Frame(const std::filesystem::path& path);

		inline void Orphan(entt::entity ent) {
			HierarchyData::Orphan(mRegistry, ent);
		}
		inline void AddChild(entt::entity parent, entt::entity newChild) {
			HierarchyData::AddChild(mRegistry, parent, newChild);
		}
		inline void SetParent(entt::entity child, entt::entity parent) {
			AddChild(parent, child);
		}
		inline entt::entity CreateEntity() {
			return CreateEntity(mRoot);
		}
		inline entt::entity GetParent(entt::entity ent) {
			return mRegistry.get<HierarchyData>(ent).mParent;
		}
		inline entt::entity GetFirstChild(entt::entity ent) {
			return mRegistry.get<HierarchyData>(ent).mFirstChild;
		}
		inline entt::entity GetLastChild(entt::entity ent) {
			return mRegistry.get<HierarchyData>(ent).mLastChild;
		}
		inline entt::entity GetNext(entt::entity ent) {
			return mRegistry.get<HierarchyData>(ent).mNext;
		}
		inline entt::entity GetPrevious(entt::entity ent) {
			return mRegistry.get<HierarchyData>(ent).mPrevious;
		}
		inline DepthFirstNodeIterator GetIterator() {
			return DepthFirstNodeIterator(&mRegistry, mRoot);
		}
		inline DepthFirstNodeDoubleIterator GetDoubleIterator() {
			return DepthFirstNodeDoubleIterator(&mRegistry, mRoot);
		}
		inline DepthFirstNodeIterator GetIterator(entt::entity subtree) {
			return DepthFirstNodeIterator(&mRegistry, subtree);
		}
		inline DepthFirstNodeDoubleIterator GetDoubleIterator(entt::entity subtree) {
			return DepthFirstNodeDoubleIterator(&mRegistry, subtree);
		}
		inline Camera& CameraData() {
			return mRegistry.get<Camera>(mCamera);
		}
		inline const Camera& CameraData() const {
			return mRegistry.get<Camera>(mCamera);
		}
		template <typename T>
		inline T& Get(entt::entity e) {
			return mRegistry.get<T>(e);
		}
		template <typename T>
		inline T* TryGet(entt::entity e) {
			return mRegistry.try_get<T>(e);
		}
		template <typename T>
		inline T& Replace(entt::entity e, const T& obj) {
			return mRegistry.replace<T>(e, obj);
		}
		template <typename T>
		inline T& Replace(entt::entity e, T&& obj) {
			return mRegistry.replace<T>(e, std::move(obj));
		}
		template <typename T, typename... Args>
		inline T& Emplace(entt::entity e, Args&&... args) {
			return mRegistry.emplace<T>(e, std::forward<Args>(args)...);
		}
		template <typename T>
		inline bool Has(entt::entity e) {
			return mRegistry.has<T>(e);
		}

		entt::meta_type GetType() const override;
		entt::meta_any GetSourceMeta() const override;
		std::filesystem::path GetPath() const override;
		void BinarySerialize(std::ostream& output, IDependencyResolver* dependencies) override;
		void BinaryDeserialize(std::istream& input, const IDependencyResolver* dependencies) override;
		void BinarySerializeReference(
			const std::filesystem::path& workingPath, 
			cereal::PortableBinaryOutputArchive& output) override;
		void BinaryDeserializeReference(
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& input) override;
		BarrierOut MoveAsync(Device device, Context context = Context()) override;
		Handle<IResource> MoveIntoHandle() override;

		void DuplicateSubframe(Frame& subframe, entt::entity e);

		friend class FrameIO;
		friend class FrameTable;
	};
}