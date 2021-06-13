#pragma once

#include <Engine/Entity.hpp>
#include <Engine/Camera.hpp>
#include <stack>

namespace Morpheus {
	class DepthFirstNodeIterator {
	private:
		std::stack<entt::entity> mNodeStack;
		entt::registry* mRegistry;

	public:
		DepthFirstNodeIterator(entt::registry* registry, entt::entity start) {
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

	struct Frame {
		entt::registry mRegistry;
		entt::entity mRoot;
		entt::entity mCamera = entt::null;

		inline entt::entity SpawnDefaultCamera(Camera** cameraOut = nullptr) {
			auto cameraNode = CreateChild(mRoot);
			auto& camera = mRegistry.emplace<Camera>(cameraNode);

			if (cameraOut)
				*cameraOut = &camera;

			return cameraNode;
		}

		inline Frame() {
			mRoot = mRegistry.create();
			mRegistry.emplace<HierarchyData>(mRoot);
		}

		inline void Orphan(entt::entity ent) {
			HierarchyData::Orphan(mRegistry, ent);
		}
		inline void AddChild(entt::entity parent, entt::entity newChild) {
			HierarchyData::AddChild(mRegistry, parent, newChild);
		}
		inline void SetParent(entt::entity child, entt::entity parent) {
			AddChild(parent, child);
		}
		inline entt::entity CreateChild(entt::entity parent) {
			auto e = mRegistry.create();
			mRegistry.emplace<HierarchyData>(e);
			AddChild(parent, e);
			return e;
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
		inline void Destroy(entt::entity ent) {
			Orphan(ent);

			for (entt::entity child = GetFirstChild(ent); 
				child != entt::null; 
				child = GetNext(child)) 
				Destroy(child);

			mRegistry.destroy(ent);
		}
	};
}