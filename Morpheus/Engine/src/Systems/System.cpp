#include <Engine/Systems/System.hpp>

namespace Morpheus {
	void SystemCollection::Startup(ITaskQueue* queue) {
		TaskGroup group;

		for (auto& system : mSystems) {
			Task task = system->Startup(*this);
			group.Adopt(std::move(task));
		}

		if (queue) {
			queue->Trigger(&group);
			queue->YieldUntilFinished(&group);
		} else {
			group();
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
			ITaskQueue* queue,
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

		mInject.SetParameters(mFrame);
		mRender.SetParameters(mSavedRenderParams);
		mUpdate.SetParameters(updateParams);

		mSavedRenderParams.mTime = time;
		mSavedRenderParams.mFrame = mFrame;

		queue->Trigger(&mInject);
		
		if (bRender) {
			queue->Trigger(&mRenderSwitch);
		} else {
			mRender.Out().SetFinishedUnsafe(true);
		}

		if (bUpdate) {
			queue->Trigger(&mUpdateSwitch);
		} else {
			mUpdate.Out().SetFinishedUnsafe(true);
		}
	}

	void FrameProcessor::Initialize(SystemCollection* systems, Frame* frame) {
		mInject.Clear();
		mUpdate.Clear();
		mRender.Clear();
		mRenderSwitch.Clear();
		mUpdateSwitch.Clear();

		Reset();

		mUpdate.In().Lock()
			.Connect(&mInject.Out())
			.Connect(&mUpdateSwitch.mOut);
		mRender.In().Lock()
			.Connect(&mInject.Out())
			.Connect(&mRenderSwitch.mOut);

		mInjectByType.clear();

		// For consistency
		mInject.Out().SetFinishedUnsafe(true);
		mUpdate.Out().SetFinishedUnsafe(true);
		mRender.Out().SetFinishedUnsafe(true);

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
		mRenderSwitch.Reset();
		mUpdateSwitch.Reset();
	}

	void FrameProcessor::AddInjector(const InjectProc& proc) {
		auto it = mInjectByType.find(proc.mTarget);

		TypeInjector* injector;

		if (it != mInjectByType.end()) {
			injector = &it->second;
		} else {
			// Create a new injector
			TypeInjector newInjector;
			newInjector.mTarget = proc.mTarget;
			
			auto emplacement = mInjectByType.emplace(proc.mTarget, newInjector);
			injector = &emplacement.first->second;

			// Inject everything
			ParameterizedTask<Frame*> task([iterator = emplacement.first](const TaskParams& e, Frame* frame) {
				for (auto& injection : iterator->second.mInjections)
					injection(frame);
			});

			mInject.Adopt(std::move(task));
		}

		// Add injector
		injector->mInjections.push_back(proc.mProc);
	}

	void FrameProcessor::Flush(ITaskQueue* queue) {
		Reset();
		mInject.SetParameters(mFrame);
		queue->Trigger(&mInject);
		queue->YieldUntilFinished(&mInject);
	}

	void FrameProcessor::AddUpdateTask(ParameterizedTask<UpdateParams>&& task) {
		mUpdate.Adopt(std::move(task));
	}

	void FrameProcessor::AddRenderTask(ParameterizedTask<RenderParams>&& task) {
		mRender.Adopt(std::move(task));
	}

	void FrameProcessor::AddRenderGroup(ParameterizedTaskGroup<RenderParams>* group) {
		mRender.Add(group);
	}
	void FrameProcessor::AddUpdateGroup(ParameterizedTaskGroup<UpdateParams>* group) {
		mUpdate.Add(group);
	}
}