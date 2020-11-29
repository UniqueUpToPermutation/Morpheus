#pragma once

#include <entt/entt.hpp>
#include <unordered_map>
#include <vector>
#include <stack>
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
	class RenderCache;
	class Renderer;

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
		inline SceneHeirarchy* GetHeirarchy();
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

		friend class SceneHeirarchy;
	};

	class NodeIterator {
	private:
		std::stack<EntityNode> mNodeStack;

	public:
		NodeIterator(EntityNode start) {
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

		inline NodeIterator& operator++() {
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

	class NodeDoubleIterator {
	private:
		std::stack<EntityNode> mNodeStack;
		IteratorDirection mDirection;

	public:
		NodeDoubleIterator(EntityNode start) {
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

		inline NodeDoubleIterator& operator++() {
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

		EntityNode mCamera;

		int mFirstFree = -1;
		int mRootsBegin = -1;
		int mRootsEnd = -1;

		RenderCache* mRenderCache = nullptr;

		void DestroyChild(int node);

		// Isolates the node from any linked lists in the tree
		void Isolate(int node);

	public:
		SceneHeirarchy(uint initialReserve = 1000);
		~SceneHeirarchy();

		EntityNode AddChild(EntityNode entityParent, entt::entity entityChild);
		EntityNode AddChild(EntityNode node);

		// Make this entity into a root
		void Clip(EntityNode entity);
		void Reparent(EntityNode entity, EntityNode newParent);
		void Destroy(EntityNode entity);

		EntityNode CreateNode(entt::entity entity);
		EntityNode CreateNode();
		EntityNode CreateChild(EntityNode parent);

		EntityNode GetRoot();

		inline NodeIterator GetIterator() {
			return NodeIterator(GetRoot());
		}

		inline NodeDoubleIterator GetDoubleIterator() {
			return NodeDoubleIterator(GetRoot());
		}

		inline RenderCache* GetRenderCache() {
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
		void BuildRenderCache(Renderer* renderer);
		void Clear();

		inline entt::registry* GetRegistry() {
			return &mRegistry;
		}

		inline entt::dispatcher* GetDispatcher() {
			return &mDispatcher;
		}

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

	SceneHeirarchy* EntityNode::GetHeirarchy() {
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
}