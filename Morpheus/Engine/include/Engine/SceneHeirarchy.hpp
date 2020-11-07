#pragma once

#include <entt/entt.hpp>
#include <unordered_map>
#include <vector>

namespace Morpheus {
	struct SceneTreeNode {
		entt::entity mEntity;
		int mNext;
		int mPrev;
		int mParent;
		int mFirstChild;
		int mLastChild;
	};

	class SceneHeirarchy;

	struct EntityNode {
	private:
		SceneHeirarchy* mTree;
		int mNode;

	public:
		EntityNode(SceneHeirarchy* tree, int node) :
			mTree(tree), mNode(node) {
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

		friend class SceneHeirarchy;
	};
	
	class SceneHeirarchy {
	private:
		std::unordered_map<entt::entity, uint> mEntityToNode;
		std::vector<SceneTreeNode> mNodes;
		entt::registry* mRegistry;
		int mFirstFree = -1;
		int mRootsBegin = -1;
		int mRootsEnd = -1;

		void DestroyChild(int node);

		// Isolates the node from any linked lists in the tree
		void Isolate(int node);

	public:
		SceneHeirarchy(entt::registry* registry, uint initialReserve = 1000);
		~SceneHeirarchy();

		EntityNode AddChild(EntityNode entityParent, entt::entity entityChild);
		EntityNode AddChild(EntityNode node);

		// Make this entity into a root
		void Clip(EntityNode entity);
		void Reparent(EntityNode entity, EntityNode newParent);
		void Destroy(EntityNode entity);

		EntityNode CreateNode(entt::entity entity);
		EntityNode CreateChild(EntityNode parent);

		void Clear();

		friend class EntityNode;
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

	entt::registry* EntityNode::GetRegistry() {
		return mTree->mRegistry;
	}
}