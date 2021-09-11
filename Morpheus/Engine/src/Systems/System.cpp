#include <Engine/Systems/System.hpp>

namespace Morpheus {

	bool SystemCollection::TryQueryInterface(
		const entt::meta_type& interfaceType,
		entt::meta_any* interfaceOut) const {
		
		auto it = mSystemInterfaces.find(interfaceType);

		if (it != mSystemInterfaces.end()) {
			*interfaceOut = it->second;
			return true;
		} 

		return false;
	}

	void SystemCollection::Startup(IComputeQueue* queue) {
		Barrier barrier;
		barrier.Node()
			.SetName("System Startup Barrier");

		for (auto& system : mSystems) {
			auto task = system->Startup(*this);
			if (task) {
				barrier.Node().After(task->OutNode());
			}
		}

		auto barrierOut = BarrierOut(barrier);

		// Run all initialization on thread queue if requested
		if (queue) {
			queue->Evaluate(barrierOut);
		} else {
			barrierOut.Evaluate();
		}

		bInitialized = true;
	}

	void SystemCollection::Shutdown() {
		mSystemInterfaces.clear();
		mSystemsByType.clear();

		for (auto& system : mSystems) {
			system->Shutdown();
		}

		bInitialized = false;
	}

	void SystemCollection::SetFrame(Frame* frame) {
		mFrameProcessor.SetFrame(frame);

		for (auto& system : mSystems) {
			system->NewFrame(frame);
		}
	}

	void FrameProcessor::Apply(const FrameTime& time, 
			IComputeQueue* queue,
			bool bUpdate,
			bool bRender) {
		UpdateParams updateParams;
		updateParams.mFrame = mFrame;
		updateParams.mTime = time;

		Reset();

		if (bFirstFrame) {
			mSavedRenderParams.mTime = time;
			mSavedRenderParams.mFrame = mFrame;
			bFirstFrame = false;
		}

		// Write inputs
		mInjectPromise = mFrame;
		mRenderPromise = mSavedRenderParams;
		mUpdatePromise = updateParams;

		mSavedRenderParams.mTime = time;
		mSavedRenderParams.mFrame = mFrame;
		
		if (bRender) {
			queue->Submit(mRender);
		} else {
			mRender.Skip();
		}

		if (bUpdate) {
			queue->Submit(mUpdate);
			queue->Submit(mInject);
		} 
	}

	void FrameProcessor::Initialize(SystemCollection* systems, Frame* frame) {
		Reset();

		mInject.After(mUpdate);
		mInject.After(mRender);

		mInjectByType.clear();

		// For consistency
		mInject.Skip();
		mUpdate.Skip();
		mRender.Skip();

		SetFrame(frame);
	}

	void FrameProcessor::SetFrame(Frame* frame) {
		mFrame = frame;
		bFirstFrame = true;
	}

	void FrameProcessor::Reset() {
		mInject.Reset();
		mRender.Reset();
		mUpdate.Reset();
	}

	void FrameProcessor::AddInjector(entt::type_info target,
		const inject_proc_t& proc, 
		int phase) {
		auto it = mInjectByType.find(target);

		TypeInjector* injector;

		if (it != mInjectByType.end()) {
			injector = &it->second;
		} else {
			// Create a new injector
			TypeInjector newInjector;
			newInjector.mTarget = target;
			
			auto emplacement = mInjectByType.emplace(target, newInjector);
			injector = &emplacement.first->second;

			FunctionPrototype<Future<Frame*>> prototype(
				[iterator = emplacement.first]
				(const TaskParams& e, Future<Frame*> frame) {
				if (iterator->second.bDirty) {
					// Sort these things to make sure that we are doing these in the
					// correct order.
					std::sort(
						iterator->second.mInjections.begin(),
						iterator->second.mInjections.end(), [](
						const TypeInjector::Injection& a, 
						const TypeInjector::Injection& b) {
						return a.mPhase < b.mPhase;
					});
					iterator->second.bDirty = false;
				}
				
				for (auto& injection : iterator->second.mInjections)
					injection.mProc(frame.Get());
			});

			mInject.Add(prototype(mInjectPromise).SetName("Injector"));
		}

		// Add injector
		injector->mInjections.push_back(
			TypeInjector::Injection{proc, phase});
		injector->bDirty = true;
	}

	void FrameProcessor::Flush(IComputeQueue* queue) {
		Reset();
		mInjectPromise.Set(mFrame);
		queue->Evaluate(mInject.OutNode());
	}

	void FrameProcessor::AddUpdateTask(TaskNode update) {
		mUpdate.Add(update);
	}

	void FrameProcessor::AddRenderTask(TaskNode node) {
		mRender.Add(node);
	}

	void FrameProcessor::AddRenderTask(std::shared_ptr<Task> task) {
		mRender.Add(task);
	}

	void FrameProcessor::AddUpdateTask(std::shared_ptr<Task> task) {
		mUpdate.Add(task);
	}
}