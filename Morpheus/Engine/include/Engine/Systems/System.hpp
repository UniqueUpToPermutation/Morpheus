#pragma once

#include "BasicMath.hpp"
#include "BasicTypes.h"
#include "Timer.hpp"

#include <vector>

#include <Engine/Defines.hpp>
#include <Engine/ThreadPool.hpp>
#include <Engine/Entity.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/Cache.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class FrameProcessor;
	class ResourceProcessor;
	class SystemCollection;
	struct GraphicsCapabilityConfig;

	typedef std::function<void(Frame*)> inject_proc_t;

	struct TypeInfoHasher {
		inline std::size_t operator()(const entt::type_info& k) const {
			return k.hash();
		}
	};

	struct MetaTypeHasher {
		inline std::size_t operator()(const entt::meta_type& k) const {
			return k.info().hash();
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
		entt::meta_type mTarget;
	};

	class IInterfaceCollection {
	public:
		virtual bool TryQueryInterface(
			const entt::meta_type& interfaceType,
			entt::meta_any* interfaceOut) const = 0;

		template <typename T>
		T* QueryInterface() const {
			entt::meta_any result;
			if (TryQueryInterface(entt::resolve<T>(), &result))
				return result.cast<T*>();
			return nullptr;
		}
	};

	class ISystem {
	public:
		virtual std::unique_ptr<Task> Startup(SystemCollection& systems) = 0;
		virtual bool IsInitialized() const = 0;
		virtual void Shutdown() = 0;
		virtual std::unique_ptr<Task> LoadResources(Frame* frame) = 0;
		virtual void NewFrame(Frame* frame) = 0;
		virtual void OnAddedTo(SystemCollection& collection) = 0;
		
		virtual ~ISystem() = default;
	};

	struct TypeInjector {
		struct Injection {
			inject_proc_t mProc;
			int mPhase;
		};

		entt::type_info mTarget;
		std::vector<Injection> mInjections;
		bool bDirty = true;
	};

	struct EnttStringHasher {
		inline std::size_t operator()(const entt::hashed_string& k) const
		{
			return k.value();
		}
	};

	typedef FunctionPrototype<Future<UpdateParams>> update_proto_t;
	typedef FunctionPrototype<Future<RenderParams>> render_proto_t;

	class FrameProcessor {
	private:
		Promise<Frame*> mInjectPromise;
		Promise<UpdateParams> mUpdatePromise;
		Promise<RenderParams> mRenderPromise;

		CustomTask mInject;
		CustomTask mUpdate;
		CustomTask mRender;

		std::unordered_map<entt::type_info, 
			TypeInjector, TypeInfoHasher> mInjectByType;

		Frame* mFrame = nullptr;
		RenderParams mSavedRenderParams;
		bool bFirstFrame = true;

		std::unordered_map<entt::hashed_string, 
			Barrier, EnttStringHasher> mBarriersByName;

		void Initialize(SystemCollection* systems, Frame* frame);

	public:
		FrameProcessor() = default;
		FrameProcessor(FrameProcessor&&) = default;
		FrameProcessor(const FrameProcessor&) = delete;
		FrameProcessor& operator=(FrameProcessor&&) = default;
		FrameProcessor& operator=(const FrameProcessor&) = delete;

		void Reset();
		void Flush(IComputeQueue* queue);
		void AddInjector(entt::type_info target,
			const inject_proc_t& proc, 
			int phase = 0);
		void AddUpdateTask(TaskNode node);
		void AddRenderTask(TaskNode node);
		void AddRenderTask(std::shared_ptr<Task> group);
		void AddUpdateTask(std::shared_ptr<Task> group);
		void Apply(const FrameTime& time, 
			IComputeQueue* queue,
			bool bUpdate = true, 
			bool bRender = true);

		void SetFrame(Frame* frame);

		inline const Promise<Frame*>& GetInjectorInput() const {
			return mInjectPromise;
		}
		inline const Promise<UpdateParams>& GetUpdateInput() const {
			return mUpdatePromise;
		}
		inline const Promise<RenderParams>& GetRenderInput() const {
			return mRenderPromise;
		}
		inline Frame* GetFrame() const {
			return mFrame;
		}
		inline Task* GetInjectGroup() {
			return &mInject;
		}
		inline Task* GetUpdateGroup() {
			return &mUpdate;
		}
		inline Task* GetRenderGroup() {
			return &mRender;
		}
		inline FrameProcessor(SystemCollection* systems) {
			Initialize(systems, nullptr);
		}
		inline void WaitOnRender(IComputeQueue* queue) {
			queue->YieldUntilFinished(mRender);
		}
		inline void WaitOnUpdate(IComputeQueue* queue) {
			queue->YieldUntilFinished(mUpdate);
		}
		inline void WaitUntilFinished(IComputeQueue* queue) {
			queue->YieldUntilFinished(mRender);
			queue->YieldUntilFinished(mUpdate);
		}
		inline void RegisterBarrier(const entt::hashed_string& str, 
			Barrier barrier) {
			mBarriersByName[str] = barrier;
		}
		inline Barrier GetBarrier(const entt::hashed_string& str) const {
			auto it = mBarriersByName.find(str);
			if (it == mBarriersByName.end()) 
				return Barrier();
			else 
				return it->second;
		}
	};

	class SystemCollection final : 
		public IInterfaceCollection {
	private:
		std::unordered_map<
			entt::meta_type, 
			entt::meta_any,
			MetaTypeHasher> mSystemsByType;

		std::unordered_map<
			entt::meta_type, 
			entt::meta_any,
			MetaTypeHasher> mSystemInterfaces;
	
		std::set<std::unique_ptr<ISystem>> mSystems;
		bool bInitialized = false;
		FrameProcessor mFrameProcessor;
		
	public:
		inline SystemCollection() : mFrameProcessor(this) {
		}

		inline void RegisterBarrier(
			const entt::hashed_string& str, 
			Barrier barrier) {
			mFrameProcessor.RegisterBarrier(str, barrier);
		}
		inline void AddRenderTask(TaskNode node) {
			mFrameProcessor.AddRenderTask(node);
		}
		inline void AddUpdateTask(TaskNode node) {
			mFrameProcessor.AddUpdateTask(node);
		}
		inline void AddRenderTask(std::shared_ptr<Task> task) {
			mFrameProcessor.AddRenderTask(task);
		}
		inline void AddUpdateTask(std::shared_ptr<Task> task) {
			mFrameProcessor.AddUpdateTask(task);
		}
		void AddInjector(entt::type_info target,
			const inject_proc_t& proc, 
			int phase = 0) {
			mFrameProcessor.AddInjector(target, proc, phase);
		}

		inline Barrier GetBarrier(
			const entt::hashed_string& str) const {
			return mFrameProcessor.GetBarrier(str);
		}

		inline FrameProcessor& GetFrameProcessor() {
			return mFrameProcessor;
		}

		template <typename T>
		inline void RegisterInterface(T* interface) {
			mSystemInterfaces[entt::resolve<T>()] = interface;
		}

		bool TryQueryInterface(
			const entt::meta_type& interfaceType,
			entt::meta_any* interfaceOut) const override;

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

		void Startup(IComputeQueue* queue = nullptr);
		void SetFrame(Frame* frame);
		Task LoadResourcesAsync(Frame* frame);
		void LoadResources(Frame* frame, IComputeQueue* queue = nullptr);
		void Shutdown();

		/* Renders and then updates the frame.
		Note that this function is asynchronous, you will need to 
		call WaitOnRender and WaitOnUpdate to wait on its completion. */
		inline void RunFrame(const FrameTime& time, 
			IComputeQueue* queue) {
			mFrameProcessor.Apply(time, queue, true, true);
		}

		/* Only updates the frame, does not render.
		Note that this function is asynchronous, you will need to 
		call WaitOnUpdate to wait on its completion. */
		inline void UpdateFrame(const FrameTime& time, 
			IComputeQueue* queue) {
			mFrameProcessor.Apply(time, queue, true, false);
		}

		/* Only renders the frame, does not update.
		Note that this function is asynchronous, you will need to 
		call WaitOnRender to wait on its completion. */
		inline void RenderFrame(const FrameTime& time, 
			IComputeQueue* queue) {
			mFrameProcessor.Apply(time, queue, false, true);
		}

		inline void WaitOnRender(IComputeQueue* queue) {
			mFrameProcessor.WaitOnRender(queue);
		}
		inline void WaitOnUpdate(IComputeQueue* queue) {
			mFrameProcessor.WaitOnUpdate(queue);
		}
		inline void WaitUntilFrameFinished(IComputeQueue* queue) {
			mFrameProcessor.WaitUntilFinished(queue);
		}

		template <typename T, typename ... Args>
		inline T* Add(Args&& ... args) {
			auto ptr = new T(args...);
			mSystemsByType[entt::resolve<T>()] = ptr;
			mSystems.emplace(ptr);
			ptr->OnAddedTo(*this);
			return ptr;
		}

		template <typename T, typename ... Args> 
		inline ISystem* AddFromFactory(Args&& ... args) {
			auto ptr = T::Create(args...);
			mSystemsByType[entt::type_id<T>()] = ptr;
			mSystems.emplace(ptr);
			ptr->OnAddedTo(*this);
			return ptr;
		}

		friend class FrameProcessor;
	};
}