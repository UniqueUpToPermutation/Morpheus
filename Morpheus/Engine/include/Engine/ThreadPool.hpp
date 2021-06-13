#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <set>
#include <memory>
#include <unordered_map>

#define ASSIGN_THREAD_ANY -1
#define ASSIGN_THREAD_MAIN 0

#ifndef NDEBUG
// #define THREAD_POOL_DEBUG
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

		inline TaskNodeInLock& Connect(Task& task) {
			return Connect(&task);
		}
		inline TaskNodeInLock& Connect(TaskNodeOut& out) {
			return Connect(&out);
		}

		inline TaskNodeInLock& Reset();
		inline uint InputsLeft() const;
		inline bool IsStarted() const;
		inline bool IsReady() const;
		inline void Clear();

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
		inline void Clear();
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

		inline void Clear() {
			mIn.Lock().Clear();
			mOut.Lock().Clear();
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

		inline bool BeginSubTask() {
			bool result = mStagesFinished <= mCurrentStage;
			++mCurrentStage;
			return result;
		}

		inline void EndSubTask() {
			++mStagesFinished;
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

	template <typename ...Args>
	class IParameterizedTask : public ITask {
	private:
		std::tuple<Args...> mArgs;

		virtual TaskResult InternalParameterizedRun(const TaskParams& params, const Args&... args) = 0;

		TaskResult InternalRun(const TaskParams& params) override {
			TaskResult result;
			std::apply([this, &params, &result](const Args&... args) {
				result = InternalParameterizedRun(params, args...);
			}, mArgs);
			return result;
		}

	public:
		inline void SetParameters(Args&&... args) {
			mArgs = std::make_tuple<Args...>(std::forward<Args...>(args)...);
		}

		inline IParameterizedTask(TaskType taskType, int assignedThread = ASSIGN_THREAD_ANY) : 
			ITask(taskType, assignedThread) {
		}
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

	template <typename T, typename... Args>
	inline auto ParameterizedTaskPerform(T& delegate, const TaskParams& params, const Args&... args) {
		return delegate(params, args...);
	}

	template <typename T, typename... Args>
	class ParameterizedLambdaTask : public IParameterizedTask<Args...> {
	private:
		T mDelegate;
		std::string mName;

		inline TaskResult PerformAndConvert(void(*func)(T& delegate, const TaskParams& params, const Args&... args), 
			const TaskParams& params, 
			const Args&... args) {
			(*func)(mDelegate, params, args...);
			return TaskResult::FINISHED;
		}

		inline TaskResult PerformAndConvert(TaskResult(*func)(T& delegate, const TaskParams& params, const Args&... args), 
			const TaskParams& params, 
			const Args&... args) {
			return (*func)(mDelegate, params, args...);
		}

	public:
		inline ParameterizedLambdaTask(T&& func, const std::string& name = "",
			TaskType type = TaskType::UNSPECIFIED,
			int assignedThread = ASSIGN_THREAD_ANY) :
			IParameterizedTask<Args...>(type, assignedThread),
			mName(name), mDelegate(std::move(func)) {
		}

		TaskResult InternalParameterizedRun(const TaskParams& params, const Args&... args) override {
			static_assert(!std::is_same_v<T, const ITask*>);
			return PerformAndConvert(&ParameterizedTaskPerform<T, Args...>,
				params, args...);
		}

		std::string GetName() const override {
			return mName;
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

	template <typename ...Args>
	struct ParameterizedTask {
	private:
		IParameterizedTask<Args...>* mPtr;

	public:
		inline ParameterizedTask() : mPtr(nullptr) {
		}

		inline ParameterizedTask(IParameterizedTask<Args...>* ptr) : mPtr(ptr) {
		}

		inline ParameterizedTask(const IParameterizedTask<Args...>* ptr) : mPtr(nullptr) {
			throw std::runtime_error("Cannot create task from const pointer!");
		}

		inline ParameterizedTask(TaskNodeIn* n) : mPtr(nullptr) {
			throw std::runtime_error("Cannot create task from TaskNodeIn!");
		}

		inline IParameterizedTask<Args...>* Ptr() {
			return mPtr;
		}

		inline const IParameterizedTask<Args...>* Ptr() const {
			return mPtr;
		}

		ParameterizedTask(const ParameterizedTask<Args...>& task) = delete;
		ParameterizedTask& operator=(const ParameterizedTask<Args...>& task) = delete;

		template <typename T>
		inline ParameterizedTask(T&& lambda, 
			const std::string& name = "",
			TaskType type = TaskType::UNSPECIFIED, 
			int assignedThread = ASSIGN_THREAD_ANY) {
			mPtr = new ParameterizedLambdaTask<T, Args...>(std::move(lambda), name, type, assignedThread);
		}

		inline void Swap(ParameterizedTask<Args...>& task) {
			std::swap(task.mPtr, mPtr);
		}

		inline ParameterizedTask(ParameterizedTask<Args...>&& task) : mPtr(nullptr) {
			Swap(task);
		}

		inline ParameterizedTask<Args...>& operator=(ParameterizedTask<Args...>&& task) {
			Swap(task);
			return *this;
		}

		~ParameterizedTask() {
			if (!mPtr) {
				delete mPtr;
			}
		}

		inline IParameterizedTask<Args...>* operator->() {
			return mPtr;
		}

		inline const IParameterizedTask<Args...>* operator->() const {
			return mPtr;
		}

		inline void operator()() {
			mPtr->operator()();
		}

		inline void SetParameters(Args&&... args) {
			mPtr->SetParameters(std::forward(args)...);
		}

		inline void SetParameters(const Args&... args) {
			mPtr->SetParameters(args...);
		}

		friend struct Task;
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

		template <typename ...Args>
		inline Task(ParameterizedTask<Args...>&& other) : 
			mPtr(other.mPtr) {
			other.mPtr = nullptr;
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

		inline operator bool() const {
			return mPtr;
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
		inline TaskGroup() {
			mBarrierOut.mIn.Lock().Connect(&mBarrierIn.mOut);
		}

		inline std::vector<ITask*>& GetExternalMembers() {
			return mMembers;
		}

		inline std::vector<Task>& GetOwnedMembers() {
			return mOwnedMembers;
		}

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
			if (member) {
				mMembers.push_back(member);

				member->In().Lock().Connect(&mBarrierIn.mOut);
				mBarrierOut.mIn.Lock().Connect(&member->Out());
			}
		}

		inline void Adopt(Task&& member) {
			if (member.Ptr()) {
				member->In().Lock().Connect(&mBarrierIn.mOut);
				mBarrierOut.mIn.Lock().Connect(&member->Out());

				mOwnedMembers.push_back(std::move(member));
			}
		}

		inline void Add(TaskGroup* subGroup) {
			if (subGroup) {
				mSubGroups.push_back(subGroup);

				subGroup->In().Lock().Connect(&mBarrierIn.mOut);
				mBarrierOut.mIn.Lock().Connect(&subGroup->Out());
			}
		}

		inline void Reset() {
			mBarrierIn.Reset();
			mBarrierOut.Reset();

			for (auto& mem : mMembers) {
				mem->Reset();
			}

			for (auto& mem : mOwnedMembers) {
				mem->Reset();
			}

			for (auto& sub : mSubGroups) {
				sub->Reset();
			}
		}

		inline void Clear() {
			mBarrierIn.Clear();
			mBarrierOut.Clear();
			mMembers.clear();
			mOwnedMembers.clear();
			mSubGroups.clear();

			mBarrierOut.mIn.Lock().Connect(&mBarrierIn.mOut);
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

	template <typename ...Args>
	class ParameterizedTaskGroup {
	private:
		std::string mName;
		TaskBarrier mBarrierIn;
		TaskBarrier mBarrierOut;
		std::vector<IParameterizedTask<Args...>*> mMembers;
		std::vector<ParameterizedTask<Args...>> mOwnedMembers;
		std::vector<ParameterizedTaskGroup<Args...>*> mSubGroups;

	public:
		inline ParameterizedTaskGroup() {
			mBarrierOut.mIn.Lock().Connect(&mBarrierIn.mOut);
		}

		inline std::vector<IParameterizedTask<Args...>*>& GetExternalMembers() {
			return mMembers;
		}

		inline std::vector<ParameterizedTask<Args...>>& GetOwnedMembers() {
			return mOwnedMembers;
		}

		inline std::vector<ParameterizedTaskGroup<Args...>>& GetSubGroups() {
			return mSubGroups;
		}

		inline void SetParameters(Args... args) {
			for (auto& mem : mMembers) {
				mem->SetParameters(std::forward<Args...>(args)...);
			}

			for (auto& mem : mOwnedMembers) {
				mem->SetParameters(std::forward<Args...>(args)...);
			}

			for (auto& group : mSubGroups) {
				group->SetParameters(std::forward<Args...>(args)...);
			}
		}

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

		inline void Add(IParameterizedTask<Args...>* member) {
			mMembers.push_back(member);

			member->In().Lock().Connect(&mBarrierIn.mOut);
			mBarrierOut.mIn.Lock().Connect(&member->Out());
		}

		inline void Adopt(ParameterizedTask<Args...>&& member) {
			member->In().Lock().Connect(&mBarrierIn.mOut);
			mBarrierOut.mIn.Lock().Connect(&member->Out());

			mOwnedMembers.push_back(std::move(member));
		}

		inline void Add(ParameterizedTaskGroup<Args...>* subGroup) {
			mSubGroups.push_back(subGroup);

			subGroup->In().Lock().Connect(&mBarrierIn.mOut);
			mBarrierOut.mIn.Lock().Connect(&subGroup->Out());
		}

		inline void Clear() {
			mBarrierIn.Clear();
			mBarrierOut.Clear();
			mMembers.clear();
			mOwnedMembers.clear();
			mSubGroups.clear();

			mBarrierOut.mIn.Lock().Connect(&mBarrierIn.mOut);
		}

		inline void Reset() {
			mBarrierIn.Reset();
			mBarrierOut.Reset();

			for (auto& mem : mMembers) {
				mem->Reset();
			}

			for (auto& mem : mOwnedMembers) {
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

	template <typename T>
	class Future;

	template <typename T>
	class Promise {
	private:
		struct Internal {
			T mData;
			TaskNodeOut mOut;
		};

		std::shared_ptr<Internal> mInternal;
		
	public:
		inline Promise() {
			mInternal = std::make_shared<Internal>();
		}

		inline TaskNodeOut* Out() {
			return &mInternal->mOut;
		}

		inline bool IsAvailable() const {
			return mInternal->mOut.Lock().IsFinished();
		}

		inline const T& Get() const {
			return mInternal->mData;
		}

		inline T& Get() {
			return mInternal->mData;
		}

		void Set(const T& value, ITaskQueue* queue);
		void Set(T&& value, ITaskQueue* queue);

		friend class Future<T>;
	};

	template <typename T>
	class Future {
	private:
		typedef typename Promise<T>::Internal Internal;

		std::shared_ptr<Internal> mInternal;

	public:

		inline uint RefCount() const {
			return mInternal.use_count();
		}

		inline Future() {
		}

		inline Future(const Promise<T>& promise) {
			mInternal = promise.mInternal;
		}

		inline TaskNodeOut* Out() {
			return &mInternal->mOut;
		}

		inline bool IsAvailable() const {
			return mInternal->mOut.Lock().IsFinished();
		}

		inline const T& Get() const {
			return mInternal->mData;
		}

		inline T& Get() {
			return mInternal->mData;
		}

		struct Comparer {
			inline bool operator()(const Future<T>& f1, const Future<T>& f2) {
				return f1.mInternal < f2.mInternal;
			}
		};
	};

	template <typename T>
	struct ResourceTask {
		Task mTask;
		Future<T> mFuture;

		inline void operator()() {
			mTask();
		}

		inline operator bool() const {
			return mTask;
		}
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
		template <typename ...Args>
		inline void Trigger(ParameterizedTaskGroup<Args...>* group);

		virtual ITask* Adopt(Task&& task) = 0;

		template <typename T>
		inline Future<T> AdoptAndTrigger(ResourceTask<T>&& task) {
			auto future = task.mFuture;
			Trigger(Adopt(std::move(task.mTask)));
			return future;
		}

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

		template <typename ...Args>
		inline void YieldUntilFinished(ParameterizedTaskGroup<Args...>* group) {
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

		template <typename T>
		inline void YieldUntil(Future<T>& future) {
			YieldUntilFinished(future.Out());
		}

		virtual void YieldUntilEmpty() = 0;

		template <typename T>
		friend class Promise;
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

	template <typename ...Args>
	void ITaskQueue::Trigger(ParameterizedTaskGroup<Args...>* group) {
		Trigger(&group->In(), false);
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

	void TaskNodeInLock::Clear() {
		mNode->bIsStarted = false;
		mNode->mInputs.clear();
		mNode->mInputsLeft = 0;
	}

	bool TaskNodeOutLock::IsFinished() const {
		return mNode->IsFinishedUnsafe();
	}

	void TaskNodeOutLock::Clear() {
		mNode->bIsFinished = false;
		mNode->mOutputs.clear();
	}

	TaskNodeOutLock& TaskNodeOutLock::Reset() {
		mNode->ResetUnsafe();
		return *this;
	}

	TaskNodeOutLock::TaskNodeOutLock(TaskNodeOut* out) : mLock(out->mMutex), mNode(out) {
	}

	template <typename T>
	void Promise<T>::Set(const T& value, ITaskQueue* queue) {
		mInternal->mData = value;
		queue->Fire(&mInternal->mOut);
	}

	template <typename T>
	void Promise<T>::Set(T&& value, ITaskQueue* queue) {
		mInternal->mData = std::move(value);
		queue->Fire(&mInternal->mOut);
	}
}