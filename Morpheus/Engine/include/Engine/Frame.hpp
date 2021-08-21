#pragma once

#include <Engine/Entity.hpp>
#include <Engine/Camera.hpp>
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

	class Frame : public IResource {
	private:
		entt::registry mRegistry;
		entt::entity mRoot;
		entt::entity mCamera = entt::null;
		std::filesystem::path mPath;

		std::unordered_set<Handle<IResource>, 
			Handle<IResource>::Hasher> mInternalResources;

	public:
		inline std::filesystem::path GetPath() const {
			return mPath;
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

		void AttachResource(Handle<IResource> resource);
		void RemoveResource(Handle<IResource> resource);

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
		void BinarySerialize(std::ostream& output) const override;
		void BinaryDeserialize(std::istream& input) override;
		void BinarySerializeSource(
			const std::filesystem::path& workingPath, 
			cereal::PortableBinaryOutputArchive& output) const override;
		void BinaryDeserializeSource(
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& input) override;
		BarrierOut MoveAsync(Device device, Context context = Context()) override;
	};
}