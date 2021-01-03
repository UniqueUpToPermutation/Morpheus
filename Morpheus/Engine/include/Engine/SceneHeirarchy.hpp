#pragma once

#include <entt/entt.hpp>
#include <unordered_map>
#include <vector>
#include <stack>

#include <Engine/EntityPrototype.hpp>

#include "btBulletDynamicsCommon.h"
#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {

	class Engine;

	struct UpdateEvent {
		double mCurrTime;
		double mElapsedTime;
		Engine* mEngine;
	};

	struct SceneTreeNode {
		entt::entity mEntity;
		int mNext;
		int mPrev;
		int mParent;
		int mFirstChild;
		int mLastChild;
	};

	class SceneHeirarchy;
	class Camera;
	class IRenderCache;
	class IRenderer;

	struct EntityNode {
	private:
		SceneHeirarchy* mTree;
		int mNode;

	public:
		EntityNode(SceneHeirarchy* tree, int node) :
			mTree(tree), mNode(node) {
		}

		static EntityNode Invalid() {
			return EntityNode(nullptr, -1);
		}

		inline entt::entity operator()() const;
		inline entt::entity GetEntity() const;
		inline SceneHeirarchy* GetScene();
		inline entt::registry* GetRegistry();
		inline void AddChild(EntityNode other);
		inline void AddChild(entt::entity other);
		inline EntityNode CreateChild();
		inline void SetParent(EntityNode other);
		inline EntityNode GetNext();
		inline EntityNode GetPrev();
		inline EntityNode GetFirstChild();
		inline EntityNode GetLastChild();
		inline EntityNode GetParent();
		inline bool IsValid() const;

		template <typename T, typename... Args>
		inline T* AddComponent(Args &&... args);

		template <typename T>
		inline T* GetComponent();

		template <typename T>
		inline T* TryGetComponent();

		inline IEntityPrototype* GetPrototype() { 
			return GetComponent<EntityPrototypeComponent>()->mPrototype;			
		}

		inline IEntityPrototype* TryGetPrototype() {
			return TryGetComponent<EntityPrototypeComponent>()->mPrototype;
		}

		inline entt::entity Clone() {
			auto prototype = GetPrototype();
			return prototype->Clone(GetEntity(), mTree);
		}

		void SetTranslation(const DG::float3& translation, bool bUpdateDescendants = true);
		void SetRotation(const DG::Quaternion& rotation, bool bUpdateDescendants = true);
		void SetScale(const DG::float3& scale, bool bUpdateDescendants = true);
		void SetTransform(const DG::float3& translation,
			const DG::float3& scale,
			const DG::Quaternion& rotation, 
			bool bUpdateDescendants = true);

		inline void SetTranslation(float x, float y, float z, bool bUpdateDescendants = true) {
			SetTranslation(DG::float3(x, y, z), bUpdateDescendants);
		}

		inline void SetScale(float x, float y, float z, bool bUpdateDescendants = true) {
			SetScale(DG::float3(x, y, z), bUpdateDescendants);
		}

		inline void SetScale(float s, bool bUpdateDescendants = true) {
			SetScale(DG::float3(s, s, s), bUpdateDescendants);
		}

		inline EntityNode Clone(EntityNode parent);

		friend class SceneHeirarchy;
	};

	class DepthFirstNodeIterator {
	private:
		std::stack<EntityNode> mNodeStack;

	public:
		DepthFirstNodeIterator(EntityNode start) {
			mNodeStack.emplace(start);
		}

		inline EntityNode operator()() {
			return mNodeStack.top();
		}

		inline EntityNode* operator->() {
			return &mNodeStack.top();
		}

		inline explicit operator bool() const {
			return !mNodeStack.empty();
		}

		inline DepthFirstNodeIterator& operator++() {
			auto& top = mNodeStack.top();

			mNodeStack.emplace(top.GetFirstChild());
			
			while (!mNodeStack.empty() && !mNodeStack.top().IsValid()) {
				mNodeStack.pop();
				if (!mNodeStack.empty()) {
					top = mNodeStack.top();
					mNodeStack.pop();
					mNodeStack.emplace(top.GetNext());
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
		std::stack<EntityNode> mNodeStack;
		IteratorDirection mDirection;

	public:
		DepthFirstNodeDoubleIterator(EntityNode start) {
			mNodeStack.emplace(start);
			mDirection = IteratorDirection::DOWN;
		}

		inline EntityNode operator()() {
			return mNodeStack.top();
		}

		inline EntityNode* operator->() {
			return &mNodeStack.top();
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
				mNodeStack.emplace(top.GetNext());
				mDirection = IteratorDirection::DOWN;
			} else {
				mNodeStack.emplace(top.GetFirstChild());
				mDirection = IteratorDirection::DOWN;
			}
			
			if (!mNodeStack.empty() && !mNodeStack.top().IsValid()) {
				mNodeStack.pop();
				if (!mNodeStack.empty()) {
					mDirection = IteratorDirection::UP;
				}
			}

			return *this;
		}
	};

	class CameraComponent;
	class Engine;

	class SceneHeirarchy {
	private:
		std::unordered_map<entt::entity, uint> mEntityToNode;
		std::vector<SceneTreeNode> mNodes;
		entt::registry mRegistry;
		entt::dispatcher mDispatcher;
		Engine* mParent = nullptr;
		btDynamicsWorld* mDynamicsWorld = nullptr;

		EntityNode mCamera;

		int mFirstFree = -1;
		int mRootsBegin = -1;
		int mRootsEnd = -1;

		IRenderCache* mRenderCache = nullptr;

		void DestroyChild(int node);

		// Isolates the node from any linked lists in the tree
		void Isolate(int node);

	public:
		SceneHeirarchy(Engine* engine, uint initialReserve = 1000);
		~SceneHeirarchy();

		inline btDynamicsWorld* GetDynamicsWorld() {
			return mDynamicsWorld;
		}

		inline btCollisionWorld* GetCollisionWorld() {
			return mDynamicsWorld;
		}

		EntityNode AddChild(EntityNode entityParent, entt::entity entityChild);
		EntityNode AddChild(EntityNode node);

		// Make this entity into a root
		void Clip(EntityNode entity);
		void Reparent(EntityNode entity, EntityNode newParent);
		void Destroy(EntityNode entity);

		void Update(double currTime, double elapsedTime);

		EntityNode CreateNode(entt::entity entity);
		EntityNode CreateNode();
		EntityNode CreateChild(EntityNode parent);

		EntityNode GetRoot();

		inline DepthFirstNodeIterator GetIterator() {
			return DepthFirstNodeIterator(GetRoot());
		}

		inline DepthFirstNodeDoubleIterator GetDoubleIterator() {
			return DepthFirstNodeDoubleIterator(GetRoot());
		}

		inline IRenderCache* GetRenderCache() {
			return mRenderCache;
		}

		Camera* GetCamera();

		EntityNode GetCameraNode() {
			return mCamera;
		}

		inline void SetCameraNode(EntityNode camera) {
			mCamera = camera;
		}

		void SetCurrentCamera(CameraComponent* component);
		void BuildRenderCache(IRenderer* renderer);
		void Clear();

		inline entt::registry* GetRegistry() {
			return &mRegistry;
		}

		inline entt::dispatcher* GetDispatcher() {
			return &mDispatcher;
		}

		// Spawns the given entity type at the root of the scene
		EntityNode Spawn(const std::string& typeName);
		// Spawns the given entity type as a child of the specified parent
		EntityNode Spawn(const std::string& typeName, EntityNode parent);

		// Spawns the given entity type at the root of the scene
		EntityNode Spawn(IEntityPrototype* prototype);
		// Spawns the given entity type as a child of the specified parent
		EntityNode Spawn(IEntityPrototype* prototype, EntityNode parent);

		friend class EntityNode;
		friend class Engine;
	};

	entt::entity EntityNode::operator()() const {
		return mTree->mNodes[mNode].mEntity;
	}

	entt::entity EntityNode::GetEntity() const {
		return mTree->mNodes[mNode].mEntity;
	}

	void EntityNode::AddChild(EntityNode other) {
		mTree->Reparent(other, *this);
	}

	void EntityNode::AddChild(entt::entity other) {
		mTree->AddChild(*this, other);
	}

	EntityNode EntityNode::CreateChild() {
		return mTree->CreateChild(*this);
	}

	void EntityNode::SetParent(EntityNode other) {
		mTree->Reparent(*this, other);
	}

	EntityNode EntityNode::GetNext() {
		return EntityNode(mTree, mTree->mNodes[mNode].mNext);
	}

	EntityNode EntityNode::GetPrev() {
		return EntityNode(mTree, mTree->mNodes[mNode].mPrev);
	}

	EntityNode EntityNode::GetFirstChild() {
		return EntityNode(mTree, mTree->mNodes[mNode].mFirstChild);
	}

	EntityNode EntityNode::GetLastChild() {
		return EntityNode(mTree, mTree->mNodes[mNode].mLastChild);
	}

	EntityNode EntityNode::GetParent() {
		return EntityNode(mTree, mTree->mNodes[mNode].mParent);
	}

	SceneHeirarchy* EntityNode::GetScene() {
		return mTree;
	}

	bool EntityNode::IsValid() const {
		return mNode >= 0;
	}

	template <typename T>
	T* EntityNode::GetComponent() {
		return &mTree->mRegistry.template get<T>(GetEntity());
	}

	template <typename T>
	T* EntityNode::TryGetComponent() {
		return mTree->mRegistry.template try_get<T>(GetEntity());
	}

	template <typename T, typename... Args>
	T* EntityNode::AddComponent(Args &&... args) {
		return &mTree->mRegistry.template emplace<T, Args...>(GetEntity(), std::forward<Args>(args)...);
	}

	EntityNode EntityNode::Clone(EntityNode parent) {
		auto prototype = GetPrototype();
		auto entity = prototype->Clone(GetEntity(), mTree);

		// Add prototype component (used for copy / spawn)
		mTree->mRegistry.emplace<EntityPrototypeComponent>(entity, prototype);
		return mTree->AddChild(parent, entity);
	}
}