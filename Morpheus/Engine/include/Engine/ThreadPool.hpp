#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <set>
#include <unordered_map>

#define ASSIGN_THREAD_ANY -1
#define ASSIGN_THREAD_MAIN 0

#ifndef NDEBUG
#define THREAD_POOL_DEBUG
#endif

typedef uint TaskId;

namespace Morpheus {
	class ThreadPool;
	class TaskSyncPoint;
	struct TaskParams;

	enum class TaskType {
		UNSPECIFIED,
		RENDER,
		UPDATE,
		FILE_IO
	};

	inline int GetTaskPriority(TaskType type) {
		switch (type) {
			case TaskType::RENDER:
				return 2;
			case TaskType::UPDATE:
				return 1;
			case TaskType::FILE_IO:
				return -1;
			default:
				return 0;
		}
	}

	class ITaskQueue;
	class TaskNodeOut;
	class TaskNodeIn;
	class ITask;
	class TaskGroup;
	struct TaskBarrier;
	struct Task;

	struct TaskParams {
		ITask* mTask;
		ITaskQueue* mQueue;
		uint mThreadId;
	};

	enum class TaskResult {
		FINISHED,
		WAITING,
		REQUEST_THREAD_SWITCH
	};

	enum class TaskPinOwnerType {
		TASK,
		BARRIER
	};

	struct TaskNodeInLock {
	private:
		std::unique_lock<std::mutex> mLock;
		TaskNodeIn* mNode;
		bool bWait = false;

	public:
		inline TaskNodeInLock(TaskNodeIn* in);

		TaskNodeInLock& Connect(TaskNodeOut* out);

		inline TaskNodeInLock& Connect(ITask* task);
		inline TaskNodeInLock& Connect(Task* task);
		inline TaskNodeInLock& Reset();
		inline uint InputsLeft() const;
		inline bool IsStarted() const;
		inline bool IsReady() const;

		inline bool ShouldWait() const {
			return bWait;
		}
	};

	class TaskNodeIn {
	private:
		bool bIsStarted = false;
		uint mInputsLeft = 0;
		std::mutex mMutex;
		std::vector<TaskNodeOut*> mInputs;
		TaskPinOwnerType mOwnerType;
		
		union {
			ITask* mTask;
			TaskBarrier* mBarrier;
		} mOwner;
	public:
		inline ITask* Task() {
			if (mOwnerType == TaskPinOwnerType::TASK) {
				return mOwner.mTask;
			} else {
				return nullptr;
			}
		}

		inline TaskBarrier* Barrier() {
			if (mOwnerType == TaskPinOwnerType::BARRIER) {
				return mOwner.mBarrier;
			} else {
				return nullptr;
			}
		}

		inline uint InputsLeftUnsafe() const {
			return mInputsLeft;
		}

		inline bool IsStartedUnsafe() const {
			return bIsStarted;
		}

		inline bool IsReadyUnsafe() const {
			return mInputsLeft == 0;
		}

		inline TaskNodeInLock Lock() {
			return TaskNodeInLock(this);
		}

		void ResetUnsafe();

		inline TaskNodeIn(ITask* owner) {
			mOwner.mTask = owner;
			mOwnerType = TaskPinOwnerType::TASK;
		}

		inline TaskNodeIn(TaskBarrier* owner) {
			mOwner.mBarrier = owner;
			mOwnerType = TaskPinOwnerType::BARRIER;
		}

		friend class TaskNodeOut;
		friend class ImmediateTaskQueue;
		friend class ThreadPool;
		friend class ITaskQueue;
		friend class TaskNodeInLock;
	};

	struct TaskNodeOutLock {
	private:
		std::unique_lock<std::mutex> mLock;
		TaskNodeOut* mNode;

	public:
		inline TaskNodeOutLock(TaskNodeOut* out);

		inline bool IsFinished() const;
		inline TaskNodeOutLock& Reset();
	};

	class TaskNodeOut {
	private:
		bool bIsFinished = false;
		std::mutex mMutex;
		std::vector<TaskNodeIn*> mOutputs;

	public:
		inline bool IsFinishedUnsafe() const {
			return bIsFinished;
		}

		inline void SetFinishedUnsafe(bool value) {
			bIsFinished = value;
		}

		void ResetUnsafe();

		inline TaskNodeOutLock Lock() {
			return TaskNodeOutLock(this);
		}

		friend class TaskNodeIn;
		friend class ImmediateTaskQueue;
		friend class ThreadPool;
		friend class ITaskQueue;
		friend class TaskNodeOutLock;
		friend class TaskNodeInLock;
	};

	struct TaskBarrier {
		TaskNodeIn mIn;
		TaskNodeOut mOut;

		inline TaskBarrier() : mIn(this) {
		}

		inline void Reset() {
			mIn.Lock().Reset();
			mOut.Lock().Reset();
		}

		// Trigger via immediate mode.
		void Trigger();
	};

	class ITask {
	private:
		int mAssignedThread;

		TaskNodeIn mNodeIn;
		TaskNodeOut mNodeOut;

		uint mStagesFinished = 0;
		uint mCurrentStage = 0;
		TaskType mTaskType;

	protected:
		virtual TaskResult InternalRun(const TaskParams& params) = 0;

	public:
		virtual ~ITask() = default;

		inline bool RequestThreadSwitch(const TaskParams& e, uint targetThread) {
			if (e.mThreadId == targetThread) 
				return false;
			else {
				mAssignedThread = targetThread;
				return true;
			}
		}

		inline bool SubTask() {
			bool result = mStagesFinished <= mCurrentStage;
			++mCurrentStage;
			if (result)
				++mStagesFinished;
			return result;
		}

		inline TaskNodeIn& In() {
			return mNodeIn;
		}

		inline TaskNodeOut& Out() {
			return mNodeOut;
		}

		virtual std::string GetName() const = 0;

		inline void Reset() {
			mStagesFinished = 0;
			mNodeIn.Lock().Reset();
			mNodeOut.Lock().Reset();
		}

		inline TaskResult Run(const TaskParams& params) {
			mCurrentStage = 0;
			return InternalRun(params);
		}

		inline bool IsFinished() {
			return mNodeOut.Lock().IsFinished();
		}

		inline bool IsStarted() {
			return mNodeIn.Lock().IsStarted();
		}

		inline bool IsReady() {
			return mNodeIn.Lock().IsReady();
		}

		inline int GetAssignedThread() const {
			return mAssignedThread;
		}

		inline TaskType GetType() const {
			return mTaskType;
		}

		ITask(TaskType taskType, int assignedThread = ASSIGN_THREAD_ANY) : 
			mAssignedThread(assignedThread),
			mTaskType(taskType),
			mNodeIn(this) {
		}

		inline void operator()();
	};

	template <typename LambdaT, bool IsVoid>
	struct TaskResultCast {
	};

	template <typename LambdaT>
	struct TaskResultCast<LambdaT, true> {
		static TaskResult Perform(LambdaT& l, const TaskParams& e) {
			l(e);
			return TaskResult::FINISHED;
		}
	};

	template <typename LambdaT>
	struct TaskResultCast<LambdaT, false> {
		static TaskResult Perform(LambdaT& l, const TaskParams& e) {
			return l(e);
		}
	};

	template <typename T>
	class LambdaTask : public ITask {
	private:
		T mDelegate;
		std::string mName;

	public:
		inline LambdaTask(T&& func, const std::string& name = "",
			TaskType type = TaskType::UNSPECIFIED,
			int assignedThread = ASSIGN_THREAD_ANY) :
			ITask(type, assignedThread),
			mName(name), mDelegate(std::move(func)) {
		}

		TaskResult InternalRun(const TaskParams& params) override {
			static_assert(!std::is_same_v<T, const ITask*>);
			return TaskResultCast<T, std::is_void<decltype(mDelegate(params))>::value>::
				Perform(mDelegate, params);
		}

		std::string GetName() const override {
			return mName;
		}
	};

	struct Task {
	private:
		ITask* mPtr;

	public:
		inline Task() : mPtr(nullptr) {
		}

		inline Task(ITask* ptr) : mPtr(ptr) {
		}

		inline Task(const ITask* ptr) : mPtr(nullptr) {
			throw std::runtime_error("Cannot create task from const pointer!");
		}

		inline Task(TaskNodeIn* n) : mPtr(nullptr) {
			throw std::runtime_error("Cannot create task from TaskNodeIn!");
		}

		inline ITask* Ptr() {
			return mPtr;
		}

		inline const ITask* Ptr() const {
			return mPtr;
		}

		Task(const Task& task) = delete;
		Task& operator=(const Task& task) = delete;

		template <typename T>
		inline Task(T&& lambda, 
			const std::string& name = "",
			TaskType type = TaskType::UNSPECIFIED, 
			int assignedThread = ASSIGN_THREAD_ANY) {
			mPtr = new LambdaTask<T>(std::move(lambda), name, type, assignedThread);
		}

		inline void Swap(Task& task) {
			std::swap(task.mPtr, mPtr);
		}

		inline Task(Task&& task) : mPtr(nullptr) {
			Swap(task);
		}

		inline Task& operator=(Task&& task) {
			Swap(task);
			return *this;
		}

		~Task() {
			if (!mPtr) {
				delete mPtr;
			}
		}

		inline ITask* operator->() {
			return mPtr;
		}

		inline const ITask* operator->() const {
			return mPtr;
		}

		inline void operator()() {
			mPtr->operator()();
		}
	};

	class TaskGroup {
	private:
		std::string mName;
		TaskBarrier mBarrierIn;
		TaskBarrier mBarrierOut;
		std::vector<ITask*> mMembers;
		std::vector<Task> mOwnedMembers;
		std::vector<TaskGroup*> mSubGroups;

	public:
		inline TaskBarrier& BarrierIn() {
			return mBarrierIn;
		}

		inline TaskBarrier& BarrierOut() {
			return mBarrierOut;
		}

		inline TaskNodeIn& In() {
			return mBarrierIn.mIn;
		}

		inline TaskNodeOut& Out() {
			return mBarrierOut.mOut;
		}

		inline void Add(ITask* member) {
			mMembers.push_back(member);

			member->In().Lock().Connect(&mBarrierIn.mOut);
			mBarrierOut.mIn.Lock().Connect(&member->Out());
		}

		inline void Adopt(Task&& member) {
			member->In().Lock().Connect(&mBarrierIn.mOut);
			mBarrierOut.mIn.Lock().Connect(&member->Out());

			mOwnedMembers.push_back(std::move(member));
		}

		inline void Add(TaskGroup* subGroup) {
			mSubGroups.push_back(subGroup);

			subGroup->In().Lock().Connect(&mBarrierIn.mOut);
			mBarrierOut.mIn.Lock().Connect(&subGroup->Out());
		}

		inline void Reset() {
			mBarrierIn.Reset();
			mBarrierOut.Reset();

			for (auto& mem : mMembers) {
				mem->Reset();
			}

			for (auto& sub : mSubGroups) {
				sub->Reset();
			}
		}

		inline bool IsFinished() {
			return Out().Lock().IsFinished();
		}

		inline bool IsStarted() {
			return In().Lock().IsStarted();
		}

		inline void operator()();

		friend class ITaskQueue;
	};

	class ITaskQueue {
	protected:
		void Trigger(TaskNodeIn* in, bool bFromNodeOut);
		void Fire(TaskNodeOut* out);
		virtual void Emplace(ITask* task) = 0;

	public:
		virtual ~ITaskQueue();

		inline void Trigger(TaskGroup* group);
		inline void Trigger(ITask* task);
		inline void Trigger(TaskBarrier* barrier);

		virtual ITask* Adopt(Task&& task) = 0;

		inline void AdoptAndTrigger(Task&& task) {
			Trigger(Adopt(std::move(task)));
		}

		virtual void YieldUntilCondition(const std::function<bool()>& predicate) = 0;

		inline void YieldFor(const std::chrono::high_resolution_clock::duration& duration) {
			auto start = std::chrono::high_resolution_clock::now();
			
			YieldUntilCondition([start, duration]() {
				auto now = std::chrono::high_resolution_clock::now();
				return (now - start) > duration;
			});
		}

		inline void YieldUntil(const std::chrono::high_resolution_clock::time_point& time) {
			YieldUntilCondition([time]() {
				auto now = std::chrono::high_resolution_clock::now();
				return now > time;
			});
		}
		
		inline void YieldUntilFinished(TaskNodeOut* out) {
			YieldUntilCondition([=]() {
				return out->Lock().IsFinished();
			});
		}

		inline void YieldUntilFinished(ITask* task) {
			YieldUntilCondition([=]() {
				return task->IsFinished();
			});
		}

		inline void YieldUntilFinished(TaskGroup* group) {
			YieldUntilCondition([=]() {
				return group->IsFinished();
			});
		}

		template <typename T>
		inline void YieldUntil(const std::future<T>& future) {
			YieldUntilCondition([&]() {
				return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
			});
		}

		virtual void YieldUntilEmpty() = 0;
	};

	class ImmediateTaskQueue : public ITaskQueue {
	private:
		std::queue<ITask*> mImmediateQueue;
		std::unordered_map<ITask*, Task> mOwnedTasks;

	protected:
		void Emplace(ITask* task) override;
		void RunOneJob();

	public:
		~ImmediateTaskQueue() {
		}

		void YieldUntilCondition(const std::function<bool()>& predicate) override;
		void YieldUntilEmpty() override;

		ITask* Adopt(Task&& task) override;
		
		inline uint RemainingJobCount() const {
			return mImmediateQueue.size();
		}
	};

	struct TaskComparePriority {
		inline bool operator()(const ITask* lhs, const ITask* rhs) {
			auto lhs_val = GetTaskPriority(lhs->GetType());
			auto rhs_val = GetTaskPriority(rhs->GetType());

			return lhs_val > rhs_val;
		}
	};

	class ThreadPool : public ITaskQueue {
	public:
		using queue_t = std::priority_queue<ITask*, std::vector<ITask*>, TaskComparePriority>;
		using gaurd_t = std::lock_guard<std::mutex>;

	private:
		bool bInitialized;
		std::vector<std::thread> mThreads;
		std::atomic<bool> bExit;

		std::vector<std::mutex> mIndividualMutexes;
		std::vector<queue_t> mIndividualQueues;
		
		std::mutex mCollectiveQueueMutex;
		queue_t mCollectiveQueue;
		
		std::mutex mOwnedTasksMutex;
		std::unordered_map<ITask*, Task> mOwnedTasks;

		queue_t mTaskQueue;

		std::atomic<uint> mTasksPending;

		void ThreadProc(bool bIsMainThread, uint threadNumber, const std::function<bool()>* finishPredicate);

	protected:
		void Emplace(ITask* task) override;

	public:
		bool IsQueueEmpty() {
			return mTasksPending == 0;
		}

		inline uint ThreadCount() const {
			return mThreads.size() + 1;
		}

		inline ThreadPool() : 
			bInitialized(false),
			bExit(false) {
			mTasksPending = 0;
		}

		~ThreadPool() {
			Shutdown();
		}

		void YieldUntilCondition(const std::function<bool()>& predicate) override;
		void YieldUntilEmpty() override;
	
		ITask* Adopt(Task&& task) override;

		void Startup(uint threads = std::thread::hardware_concurrency());
		void Shutdown();

		friend class TaskQueueInterface;
		friend class TaskBarrier;
		friend class TaskNodeDependencies;
	};

	void ITask::operator()() {
		ImmediateTaskQueue queue;
		queue.Trigger(this);
		queue.YieldUntilEmpty();
	}

	void TaskGroup::operator()() {
		ImmediateTaskQueue queue;
		queue.Trigger(&mBarrierIn);
		queue.YieldUntilEmpty();
	}

	void ITaskQueue::Trigger(TaskGroup* group) {
		Trigger(&group->In(), false);
	}

	void ITaskQueue::Trigger(ITask* task) {
		if (task) {
			Trigger(&task->In(), false);
		}
	}

	void ITaskQueue::Trigger(TaskBarrier* barrier) {
		Trigger(&barrier->mIn, false);
	}

	TaskNodeInLock::TaskNodeInLock(TaskNodeIn* in) : mLock(in->mMutex), mNode(in) {
	}

	TaskNodeInLock& TaskNodeInLock::Connect(ITask* task) {
		if (task)
			return Connect(&task->Out());
		else 
			return *this;
	}

	TaskNodeInLock& TaskNodeInLock::Connect(Task* task) {
		return Connect(task->Ptr());
	}

	TaskNodeInLock& TaskNodeInLock::Reset() {
		mNode->ResetUnsafe();
		return *this;
	}
	
	uint TaskNodeInLock::InputsLeft() const {
		return mNode->InputsLeftUnsafe();
	}

	bool TaskNodeInLock::IsStarted() const {
		return mNode->IsStartedUnsafe();
	}

	bool TaskNodeInLock::IsReady() const {
		return mNode->IsReadyUnsafe();
	}

	bool TaskNodeOutLock::IsFinished() const {
		return mNode->IsFinishedUnsafe();
	}

	TaskNodeOutLock& TaskNodeOutLock::Reset() {
		mNode->ResetUnsafe();
		return *this;
	}

	TaskNodeOutLock::TaskNodeOutLock(TaskNodeOut* out) : mLock(out->mMutex), mNode(out) {
	}
}