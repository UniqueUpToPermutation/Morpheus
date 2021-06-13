#pragma once

#include "BasicMath.hpp"
#include "NativeAppBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "EngineFactory.h"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "ScreenCapture.hpp"
#include "Image.h"
#include "Timer.hpp"

#include <vector>
#include <Engine/Entity.hpp>
#include <Engine/ThreadPool.hpp>
#include <Engine/Frame.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/GeometryStructures.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class FrameProcessor;
	class ResourceProcessor;
	class SystemCollection;
	struct GraphicsCapabilityConfig;

	typedef std::function<void(Frame*)> inject_proc_t;

	struct TypeInfoHasher {
		inline std::size_t operator()(const entt::type_info& k) const
		{
			return k.hash();
		}
	};

	struct FrameTime {
		double mCurrentTime = 0.0;
		double mElapsedTime = 0.0;

		inline FrameTime() {
		}

		inline FrameTime(DG::Timer& timer) {
			mCurrentTime = timer.GetElapsedTime();
			mElapsedTime = 0.0;
		}

		inline void UpdateFrom(DG::Timer& timer) {
			double last = mCurrentTime;
			mCurrentTime = timer.GetElapsedTime();
			mElapsedTime = mCurrentTime - last;
		}
	};

	struct UpdateParams {
		FrameTime mTime;
		Frame* mFrame;
	};

	struct RenderParams {
		FrameTime mTime;
		Frame* mFrame;
	};

	struct InjectProc {
		inject_proc_t mProc;
		entt::type_info mTarget;
	};

	template <typename T>
	class IResourceCache {
	public:
		virtual Future<T*> Load(const LoadParams<T>& params, ITaskQueue* queue) = 0;
	};

	class ISystem {
	public:
		virtual Task Startup(SystemCollection& systems) = 0;
		virtual bool IsInitialized() const = 0;
		virtual void Shutdown() = 0;
		virtual void NewFrame(Frame* frame) = 0;
		virtual void OnAddedTo(SystemCollection& collection) = 0;
		
		virtual ~ISystem() = default;
	};

	struct TypeInjector {
		entt::type_info mTarget;
		std::vector<std::function<void(Frame*)>> mInjections;
	};

	class FrameProcessor {
	private:
		ParameterizedTaskGroup<Frame*> mInject;
		ParameterizedTaskGroup<UpdateParams> mUpdate;
		ParameterizedTaskGroup<RenderParams> mRender;
		TaskBarrier mRenderSwitch;
		TaskBarrier mUpdateSwitch;

		std::unordered_map<entt::type_info, 
			TypeInjector, TypeInfoHasher> mInjectByType;

		Frame* mFrame = nullptr;
		bool bFirstFrame = true;
		RenderParams mSavedRenderParams;

		void Initialize(SystemCollection* systems, Frame* frame);

	public:
		void Reset();
		void Flush(ITaskQueue* queue);
		void AddInjector(const InjectProc& proc);
		void AddUpdateTask(ParameterizedTask<UpdateParams>&& task);
		void AddRenderTask(ParameterizedTask<RenderParams>&& task);
		void Apply(const FrameTime& time, 
			ITaskQueue* queue,
			bool bUpdate = true, 
			bool bRender = true);

		void SetFrame(Frame* frame);
		inline bool IsFirstFrame() const {
			return bFirstFrame;
		}
		inline Frame* GetFrame() const {
			return mFrame;
		}
		inline ParameterizedTaskGroup<Frame*>& GetInjectGroup() {
			return mInject;
		}
		inline ParameterizedTaskGroup<UpdateParams>& GetUpdateGroup() {
			return mUpdate;
		}
		inline ParameterizedTaskGroup<RenderParams>& GetRenderGroup() {
			return mRender;
		}
		inline FrameProcessor(SystemCollection* systems) {
			Initialize(systems, nullptr);
		}
		inline void WaitOnRender(ITaskQueue* queue) {
			queue->YieldUntilFinished(&mRender);
		}
		inline void WaitOnUpdate(ITaskQueue* queue) {
			queue->YieldUntilFinished(&mUpdate);
		}
	};

	struct EnttStringHasher {
		inline std::size_t operator()(const entt::hashed_string& k) const
		{
			return k.value();
		}
	};

	class SystemCollection {
	private:
		std::unordered_map<entt::hashed_string, TaskBarrier*, EnttStringHasher> mBarriersByName;
		std::unordered_map<entt::hashed_string, 
			ParameterizedTaskGroup<UpdateParams>*, EnttStringHasher> mUpdateGroupsByName;
		std::unordered_map<entt::hashed_string,
			ParameterizedTaskGroup<RenderParams>*, EnttStringHasher> mRenderGroupsByName;
		std::set<std::unique_ptr<ISystem>> mSystems;
		std::unordered_map<entt::type_info, entt::meta_any, TypeInfoHasher> mSystemsByType;
		std::unordered_map<entt::type_info, entt::meta_any, TypeInfoHasher> mSystemInterfaces;
		bool bInitialized = false;
		FrameProcessor mFrameProcessor;
		
	public:
		inline SystemCollection() : mFrameProcessor(this) {
		}

		inline void RegisterUpdateGroup(const entt::hashed_string& str, ParameterizedTaskGroup<UpdateParams>* group) {
			mUpdateGroupsByName[str] = group;
		}

		inline void RegisterRenderGroup(const entt::hashed_string& str, ParameterizedTaskGroup<RenderParams>* group) {
			mRenderGroupsByName[str] = group;
		}

		inline void RegisterBarrier(const entt::hashed_string& str, TaskBarrier* barrier) {
			mBarriersByName[str] = barrier;
		}

		inline ParameterizedTaskGroup<UpdateParams>* GetUpdateGroup(const entt::hashed_string& str) const {
			auto it = mUpdateGroupsByName.find(str);
			if (it == mUpdateGroupsByName.end())
				return nullptr;
			else
				return it->second;
		}

		inline ParameterizedTaskGroup<RenderParams>* GetRenderGroup(const entt::hashed_string& str) const {
			auto it = mRenderGroupsByName.find(str);
			if (it == mRenderGroupsByName.end()) 
				return nullptr;
			else 
				return it->second;
		}

		inline TaskBarrier* GetBarrier(const entt::hashed_string& str) const {
			auto it = mBarriersByName.find(str);
			if (it == mBarriersByName.end()) 
				return nullptr;
			else 
				return it->second;
		}

		inline FrameProcessor& GetFrameProcessor() {
			return mFrameProcessor;
		}

		template <typename T>
		inline void RegisterInterface(T* interface) {
			mSystemInterfaces[entt::type_id<T>()] = interface;
		}

		template <typename T>
		T* QueryInterface() const {
			auto it = mSystemInterfaces.find(entt::type_id<T>());
			if (it != mSystemInterfaces.end()) {
				return it->second.template cast<T*>();
			} else {
				return nullptr;
			}
		}

		template <typename T>
		inline void AddCacheInterface(IResourceCache<T>* cache) {
			RegisterInterface<IResourceCache<T>>(cache);
		}

		template <typename T>
		inline IResourceCache<T>* GetCache() const {
			return QueryInterface<IResourceCache<T>>();
		}

		template <typename T>
		inline Future<T*> Load(const LoadParams<T>& params, ITaskQueue* queue) const {
			return GetCache<T>()->Load(params, queue);
		}

		inline bool IsInitialized() const {
			return bInitialized;
		}

		inline const std::set<std::unique_ptr<ISystem>>& Systems() const {
			return mSystems;
		}

		template <typename T>
		inline T* GetSystem() const {
			auto it = mSystemsByType.find(entt::type_id<T>());
			if (it != mSystemsByType.end()) {
				return it->second.template cast<T*>();
			} else {
				return nullptr;
			}
		}

		void Startup(ITaskQueue* queue = nullptr);
		void SetFrame(Frame* frame);
		void Shutdown();

		inline void RunFrame(const FrameTime& time, 
			ITaskQueue* queue,
			bool bUpdate = true, 
			bool bRender = true) {
			mFrameProcessor.Apply(time, queue, bUpdate, bRender);
		}

		inline void WaitOnRender(ITaskQueue* queue) {
			mFrameProcessor.WaitOnRender(queue);
		}
		inline void WaitOnUpdate(ITaskQueue* queue) {
			mFrameProcessor.WaitOnUpdate(queue);
		}

		template <typename T, typename ... Args>
		inline T* Add(Args&& ... args) {
			auto ptr = new T(args...);
			mSystemsByType[entt::type_id<T>()] = ptr;
			mSystems.emplace(ptr);
			ptr->OnAddedTo(*this);
			return ptr;
		}

		friend class FrameProcessor;
	};
}