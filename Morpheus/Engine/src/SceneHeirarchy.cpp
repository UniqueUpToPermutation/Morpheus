#include <Engine/SceneHeirarchy.hpp>
#include <Engine/Engine.hpp>
#include <Engine/Camera.hpp>
#include <Engine/PhysicsComponents.hpp>
#include <Engine/Transform.hpp>

namespace Morpheus {
	SceneHeirarchy::SceneHeirarchy(Engine* parent, uint initialReserve) :
		mParent(parent),
		mCamera(EntityNode::Invalid()) {
		
		mNodes.reserve(initialReserve);
		
		// Create root
		auto root = CreateNode();

		// Create camera
		auto cameraNode = root.CreateChild();
		cameraNode.AddComponent<Camera>();

		mCamera = cameraNode;
		mDynamicsWorld = parent->CreateDynamicsWorld();
	}

	SceneHeirarchy::~SceneHeirarchy() {
		if (mDynamicsWorld) {
			delete mDynamicsWorld;
			mDynamicsWorld = nullptr;
		}

		Clear();
	}

	void SceneHeirarchy::Clear() {
		for (auto id = mRootsBegin; id != -1;) {
			auto to_destroy = id;
			id = mNodes[id].mNext;
			Destroy(EntityNode(this, to_destroy));
		}

		if (mRenderCache) {
			delete mRenderCache;
			mRenderCache = nullptr;
		}
	}

	void SceneHeirarchy::BuildRenderCache(IRenderer* renderer) {
		if (mRenderCache) {
			delete mRenderCache;
		}

		mRenderCache = renderer->BuildRenderCache(this);
	}

	Camera* SceneHeirarchy::GetCamera() {
		return mCamera.GetComponent<Camera>();
	}

	EntityNode SceneHeirarchy::AddChild(EntityNode entityParent, entt::entity entityChild) {
		auto& node_parent = mNodes[entityParent.mNode];

		if (mFirstFree == -1) {
			EntityNode result(this, mNodes.size());

			SceneTreeNode node;
			node.mEntity = entityChild;
			node.mFirstChild = -1;
			node.mLastChild = -1;
			node.mPrev = -1;
			node.mNext = node_parent.mFirstChild;
			if (node_parent.mFirstChild != -1) {
				mNodes[node_parent.mFirstChild].mPrev = result.mNode;
			}
			node_parent.mFirstChild = result.mNode;
			node.mParent = entityParent.mNode;

			mNodes.emplace_back(node);
			return result;
		} else {
			EntityNode result(this, mFirstFree);

			auto& firstFree = mNodes[mFirstFree];

			// Pop from free stack
			mFirstFree = firstFree.mNext;

			firstFree.mEntity = entityChild;
			firstFree.mFirstChild = -1;
			firstFree.mFirstChild = -1;
			firstFree.mPrev = -1;
			firstFree.mNext = node_parent.mFirstChild;
			if (node_parent.mFirstChild != -1) {
				mNodes[node_parent.mFirstChild].mPrev = result.mNode;
			}
			node_parent.mFirstChild = result.mNode;
			firstFree.mParent = entityParent.mNode;
			return result;
		}
	}

	EntityNode SceneHeirarchy::AddChild(EntityNode node) {
		auto entity = mRegistry.create();
		return AddChild(node, entity);
	}

	void SceneHeirarchy::Clip(EntityNode entity) {
		auto& entity_node = mNodes[entity.mNode];

		// Remove the node from the linked list
		Isolate(entity.mNode);

		auto prevRootBegin = mRootsBegin;

		if (mRootsBegin != -1) {
			mNodes[mRootsBegin].mPrev = entity.mNode;
		}

		mRootsBegin = entity.mNode;

		if (mRootsEnd == -1) {
			mRootsEnd = entity.mNode;
		}

		entity_node.mNext = prevRootBegin;
		entity_node.mParent = -1;
		entity_node.mPrev = -1;
	}

	void SceneHeirarchy::Reparent(EntityNode entity, EntityNode newParent) {
		auto& entity_node = mNodes[entity.mNode];
		auto& new_parent_node = mNodes[newParent.mNode];

		// Remove the node from the linked list
		Isolate(entity.mNode);
	
		entity_node.mFirstChild = -1;
		entity_node.mFirstChild = -1;
		entity_node.mPrev = -1;
		entity_node.mNext = new_parent_node.mFirstChild;

		if (new_parent_node.mFirstChild != -1) {
			mNodes[new_parent_node.mFirstChild].mPrev = entity.mNode;
		} else {
			new_parent_node.mLastChild = entity.mNode;
		}

		new_parent_node.mFirstChild = entity.mNode;
		entity_node.mParent = newParent.mNode;
	}

	void SceneHeirarchy::Destroy(EntityNode entity) {
		auto& node = mNodes[entity.mNode];
		mRegistry.destroy(node.mEntity);
		
		auto prev = node.mPrev;
		auto next = node.mNext;
		auto parent = node.mParent;

		if (parent >= 0) {
			if (prev >= 0) {
				mNodes[prev].mNext = next;
			} else {
				mNodes[parent].mFirstChild = next;
			}

			if (next >= 0) {
				mNodes[next].mPrev = prev;
			} else {
				mNodes[parent].mLastChild = prev;
			}
		} else {
			if (prev >= 0) {
				mNodes[prev].mNext = next;
			} else {
				mRootsBegin = next;
			}

			if (next >= 0) {
				mNodes[next].mPrev = prev;
			} else {
				mRootsEnd = prev;
			}
		}

		auto firstChild = node.mFirstChild;
		for (auto child = firstChild; child != -1;) {
			// Recursively destroy everything below without topology updates
			auto next = mNodes[child].mNext;
			DestroyChild(child); 
			child = next;
		}

		// Free up this node
		node.mNext = mFirstFree;
		mFirstFree = entity.mNode;
	}

	EntityNode SceneHeirarchy::GetRoot() {
		return EntityNode(this, mRootsBegin);
	}

	EntityNode SceneHeirarchy::CreateNode(entt::entity entity) {
		if (mFirstFree == -1) {
			EntityNode result(this, mNodes.size());

			SceneTreeNode node;
			node.mEntity = entity;
			node.mFirstChild = -1;
			node.mLastChild = -1;
			node.mPrev = mRootsEnd;
			node.mNext = -1;

			if (mRootsEnd != -1) {
				mNodes[mRootsEnd].mNext = result.mNode;
			}

			node.mParent = -1;
			mRootsEnd = result.mNode;

			if (mRootsBegin == -1) {
				mRootsBegin = result.mNode;
			}

			mNodes.emplace_back(node);
			return result;
		} else {
			EntityNode result(this, mFirstFree);

			auto& firstFree = mNodes[mFirstFree];

			// Pop from free stack
			mFirstFree = firstFree.mNext;
			firstFree.mEntity = entity;
			firstFree.mFirstChild = -1;
			firstFree.mLastChild = -1;
			firstFree.mPrev = mRootsEnd;
			firstFree.mNext = -1;

			if (mRootsEnd != -1) {
				mNodes[mRootsEnd].mNext = result.mNode;
			}

			firstFree.mParent = -1;
			mRootsBegin = result.mNode;

			if (mRootsBegin == -1) {
				mRootsBegin = result.mNode;
			}

			return result;
		}
	}

	EntityNode SceneHeirarchy::CreateNode() {
		auto e = mRegistry.create();
		return CreateNode(e);
	}

	EntityNode SceneHeirarchy::CreateChild(EntityNode parent) {
		EntityNode node = CreateNode();
		parent.AddChild(node);
		return node;
	}

	void SceneHeirarchy::Isolate(int node) {
		auto& entity_node = mNodes[node];
		if (entity_node.mPrev != -1) {
			auto& prev = mNodes[entity_node.mPrev];
			prev.mNext = entity_node.mNext;
		} else if (entity_node.mParent != -1) {
			auto& parent = mNodes[entity_node.mParent];
			parent.mFirstChild = entity_node.mNext;
		} else {
			mRootsBegin = entity_node.mNext;
		}

		if (entity_node.mNext != -1) {
			auto& next = mNodes[entity_node.mNext];
			next.mPrev = entity_node.mPrev;
		} else if (entity_node.mParent != -1) {
			auto& parent = mNodes[entity_node.mParent];
			parent.mLastChild = entity_node.mPrev;
		}
		else {
			mRootsEnd = entity_node.mPrev;
		}
	}

	void SceneHeirarchy::DestroyChild(int node) {
		auto& node_ = mNodes[node];

		for (auto child = node_.mFirstChild; child != -1; child = mNodes[child].mNext) {
			DestroyChild(child);
		}

		mRegistry.destroy(node_.mEntity);

		// Free up this node
		node_.mNext = mFirstFree;
		mFirstFree = node;
	}

	EntityNode SceneHeirarchy::Spawn(const std::string& typeName) {
		auto entity = mParent->mEntityPrototypes.Spawn(typeName, mParent, this);
		return AddChild(GetRoot(), entity);
	}

	EntityNode SceneHeirarchy::Spawn(IEntityPrototype* prototype) {
		auto entity = prototype->Spawn(mParent, this);
		return AddChild(GetRoot(), entity);
	}

	EntityNode SceneHeirarchy::Spawn(const std::string& typeName, EntityNode parent) {
		auto entity = mParent->mEntityPrototypes.Spawn(typeName, mParent, this);
		return AddChild(parent, entity);
	}

	EntityNode SceneHeirarchy::Spawn(IEntityPrototype* prototype, EntityNode parent) {
		auto entity = prototype->Spawn(mParent, this);
		return AddChild(parent, entity);
	}

	void SceneHeirarchy::Update(double currTime, double elapsedTime) {
		mDynamicsWorld->stepSimulation(1.f / 60.f);

		// Copy over all active physics objects
		auto rbView = mRegistry.view<RigidBodyComponent, Transform>();

		for (auto en : rbView) {
			auto& rbComponent = rbView.get<RigidBodyComponent>(en);
			auto& transform = rbView.get<Transform>(en);

			if (rbComponent.GetRigidBody()->isActive()) {
				transform.UpdateCacheFromMotionState(rbComponent.GetMotionState());
			}
		}

		// Update everything else that needs updating
		UpdateEvent e;
		e.mCurrTime = currTime;
		e.mElapsedTime = elapsedTime;
		e.mEngine = mParent;

		mDispatcher.trigger<UpdateEvent>(e);
	}

}