#pragma once

#include <unordered_map>
#include <vector>
#include <stack>

#include <Engine/Entity.hpp>
#include <Engine/ImGuiObject.hpp>

#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {

	class Engine;
	class Scene;
	class Camera;
	class IRenderer;

	struct FrameBeginEvent {
		Scene* mScene;
		IRenderer* mRenderer;
	};

	struct UpdateEvent {
		Scene* mSender;
		double mCurrTime;
		double mElapsedTime;
	};

	struct SceneBeginEvent {
		Scene* mSender;
	};

	class ISystem {
	public:
		virtual void Startup(Scene* scene) = 0;
		virtual void Shutdown(Scene* scene) = 0;
		virtual void OnSceneBegin(const SceneBeginEvent& args) = 0;
		virtual void OnFrameBegin(const FrameBeginEvent& args) = 0;
		virtual void OnSceneUpdate(const UpdateEvent& e) = 0;

		virtual ~ISystem() {
		}
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

	using system_id = entt::family<struct system_id_tag>;

	class Scene {
	private:
		bool bInitializedByEngine = false;
		bool bBeginCalled = false;

		entt::registry mRegistry;
		entt::dispatcher mDispatcher;
		std::map<
			decltype(system_id::type<ISystem>), 
			ISystem*> mSystems;

		EntityNode mCamera;
		EntityNode mRoot;

		std::set<IImGuiObject*> mImGuiObjects;
		
	public:
		Scene();
		~Scene();

		template <typename GuiT, typename ... T>
		GuiT* AddImGuiObject(T ... args) {
			auto guiObj = new GuiT(args...);
			guiObj->OnCreate(this);
			mImGuiObjects.emplace(guiObj);
			return guiObj;
		}

		void DestroyImGuiObject(IImGuiObject* guiObject) {
			mImGuiObjects.erase(guiObject);
			guiObject->OnDestroy(this);
			delete guiObject;
		}

		template <typename SystemT, typename ... T>
		SystemT* AddSystem(T ... args) {
			auto system = new SystemT(args...);
			system->Startup(this);
			mSystems.emplace(system_id::type<SystemT>, system);
			return system;
		}

		template <typename SystemT>
		SystemT* GetSystem() {
			return dynamic_cast<SystemT*>(mSystems[system_id::type<SystemT>]);
		}

		template <typename SystemT>
		SystemT* TryGetSystem() {
			auto it = mSystems.find(system_id::type<SystemT>);

			if (it != mSystems.end()) {
				return dynamic_cast<SystemT*>(it->second);
			} else {
				return nullptr;
			}
		}

		template <typename EventArgs>
		inline void Trigger(const EventArgs& eventArgs) {
			mDispatcher.trigger<EventArgs>(eventArgs);
		}

		void Begin();
		void Shutdown();
		void Update(double currTime, double elapsedTime);
		void BeginFrame(const FrameBeginEvent& e);

		EntityNode CreateNode(entt::entity entity);
		EntityNode CreateNode();
		EntityNode GetRoot();

		inline DepthFirstNodeIterator GetIterator() {
			return DepthFirstNodeIterator(GetRoot());
		}

		inline DepthFirstNodeDoubleIterator GetDoubleIterator() {
			return DepthFirstNodeDoubleIterator(GetRoot());
		}

		Camera* GetCamera();

		EntityNode GetCameraNode() {
			return mCamera;
		}

		inline void SetCameraNode(EntityNode camera) {
			mCamera = camera;
		}

		void BuildRenderCache(IRenderer* renderer);
		void Clear();

		inline entt::registry* GetRegistry() {
			return &mRegistry;
		}

		inline entt::dispatcher* GetDispatcher() {
			return &mDispatcher;
		}

		inline bool IsInitializedByEngine() const {
			return bInitializedByEngine;
		}

		inline bool HasBegun() const {
			return bBeginCalled;
		}

		friend class EntityNode;
		friend class Engine;
	};
}