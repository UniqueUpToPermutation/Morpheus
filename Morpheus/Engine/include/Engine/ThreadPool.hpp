#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <random>
#include <unordered_map>
#include <future>

#include <Engine/Defines.hpp>
#include <Engine/Graph.hpp>
#include <Engine/Entity.hpp>

#define ASSIGN_THREAD_IO -2
#define ASSIGN_THREAD_ANY -1
#define ASSIGN_THREAD_MAIN 0

#define TASK_NONE -1 
#define PIPE_NONE -1
#define BARRIER_NONE -1

namespace Morpheus {
	typedef int TaskNodeId;

	enum class TaskNodeType {
		UNKNOWN_NODE,
		TASK_NODE,
		PIPE_NODE,
		BARRIER_NODE
	};

	struct TaskId {
		TaskNodeId mId;

		inline TaskId() :
			mId(TASK_NONE) {	
		}

		inline TaskId(TaskNodeId id) :
			mId(id) {
		}

		inline bool IsValid() const {
			return mId >= 0;
		}
	};

	struct PipeId {
		TaskNodeId mId;

		inline PipeId() :
			mId(PIPE_NONE) {
		}

		inline PipeId(TaskNodeId id) :
			mId(id) {
		}

		inline bool IsValid() const {
			return mId >= 0;
		}
	};

	struct BarrierId {
		TaskNodeId mId;

		inline BarrierId() :
			mId(BARRIER_NONE) {
		}

		inline BarrierId(TaskNodeId id) :
			mId(id) {
		}

		inline bool IsValid() const {
			return mId >= 0;
		}
	};

	class ThreadPool;
	class TaskBarrier;

	struct TaskParams {
		ThreadPool* mPool;
		TaskBarrier* mBarrier;
		uint mThreadId;
		TaskNodeId mTaskId;
	};

	typedef std::function<void(const TaskParams&)> TaskFunc;
	typedef std::function<void(ThreadPool*)> TaskBarrierCallback;

	class TaskDesc {
	public:
		TaskBarrier* mBarrier;
		int mAssignedThread;
		TaskFunc mFunc;

		TaskDesc(TaskFunc func, TaskBarrier* barrier = nullptr, int assignedThread = ASSIGN_THREAD_ANY) :
			mFunc(func),
			mBarrier(barrier),
			mAssignedThread(assignedThread) {
		}
	};

	struct TaskNodeData {
		TaskNodeType mType;
		int mInputsLeftToCollect;
		int mOutputsLeftToTrigger;

		inline TaskNodeData(TaskNodeType type) :
			mType(type),
			mInputsLeftToCollect(0),
			mOutputsLeftToTrigger(0) {
		}

		inline TaskNodeData() : TaskNodeData(TaskNodeType::UNKNOWN_NODE) {
		}
	};

	class TaskNodeDependencies {
	private:
		std::unordered_map<TaskNodeId, TaskNodeData>::iterator mIterator;
		ThreadPool* mPool;
		TaskNodeId mId;

		void After(TaskNodeId other);

	public:
		TaskNodeDependencies(TaskNodeId id, 
			ThreadPool* pool, 
			const std::unordered_map<TaskNodeId, TaskNodeData>::iterator& it) :
			mId(id),
			mPool(pool),
			mIterator(it) {
		}

		inline TaskNodeDependencies& After(TaskId other) {
			if (other.IsValid())
				After(other.mId);
			return *this;
		}
		inline TaskNodeDependencies& After(PipeId other) {
			if (other.IsValid())
				After(other.mId);
			return *this;
		}
		TaskNodeDependencies& After(TaskBarrier* barrier);
	};

	class Task {
	private:
		TaskBarrier* mBarrier;

	public:
		int mAssignedThread;
		TaskNodeId mId;
		TaskFunc mFunc;

		inline Task(TaskFunc func, TaskNodeId id, TaskBarrier* barrier = nullptr, 
			int assignedThread = ASSIGN_THREAD_ANY) :
			mFunc(func),
			mId(id),
			mBarrier(barrier),
			mAssignedThread(assignedThread) {
		}

		inline Task(const TaskDesc& task, TaskNodeId id) :
			mFunc(task.mFunc),
			mId(id),
			mBarrier(task.mBarrier),
			mAssignedThread(task.mAssignedThread) {
		}

		inline Task() :
			mFunc(nullptr),
			mId(0),
			mBarrier(nullptr),
			mAssignedThread(ASSIGN_THREAD_ANY) {
		}

		friend class ThreadPool;
		friend class TaskQueueInterface;
	};

	class ThreadPipe {
	private:
		TaskNodeId mId;
		std::promise<entt::meta_any> mPromise;
		std::shared_future<entt::meta_any> mFuture;

	public:
		inline ThreadPipe(TaskNodeId id) :
			mId(id) {
			mFuture = mPromise.get_future();
		}

		inline ThreadPipe(ThreadPipe&& other) noexcept :
			mId(other.mId),
			mPromise(std::move(other.mPromise)),
			mFuture(std::move(other.mFuture)) {
		}

		inline void Write(entt::meta_any&& any) {
			mPromise.set_value(std::move(any));
		}

		inline void WriteException(std::exception_ptr ex) {
			mPromise.set_exception(ex);
		}

		inline const entt::meta_any& Read() const {
			return mFuture.get();
		}

		friend class ThreadPool;
		friend class TaskQueueInterface;
	};

	class TaskQueueInterface {
	private:
		std::unique_lock<std::mutex> mLock;
		std::unique_lock<std::mutex> mIOLock;
		ThreadPool* mPool;

	public:
		inline TaskQueueInterface(std::unique_lock<std::mutex>&& lock,
			std::unique_lock<std::mutex>&& ioLock,
			ThreadPool* pool) :
			mLock(std::move(lock)),
			mIOLock(std::move(ioLock)),
			mPool(pool) {
		}

		TaskQueueInterface(const TaskQueueInterface& other) = delete;

		inline TaskQueueInterface(TaskQueueInterface&& other) noexcept {
			std::swap(mLock, other.mLock);
			std::swap(mPool, other.mPool);
			std::swap(mIOLock, other.mIOLock);
		}

		PipeId MakePipe();
		TaskId MakeTask(const TaskDesc& desc);

		inline TaskId MakeTask(const TaskFunc& func, 
			TaskBarrier* barrier = nullptr, 
			int assignedThread = ASSIGN_THREAD_ANY) {
			return MakeTask(TaskDesc(func, barrier, assignedThread));
		}

		inline TaskId MakeIOTask(const TaskFunc& func, TaskBarrier* barrier = nullptr) {
			return MakeTask(TaskDesc(func, barrier, ASSIGN_THREAD_IO));
		}

		TaskNodeDependencies Dependencies(TaskId task);
		TaskNodeDependencies Dependencies(PipeId pipe);

		void Schedule(TaskId task);
	};

	class TaskBarrier {
	private:
		std::atomic<uint> mTaskCount = 0;
		TaskBarrierCallback mOnReached;
		BarrierId mNodeId = -1;

	public:
		inline TaskBarrier() {
		}

		inline TaskBarrier(const TaskBarrierCallback& onReachedCallback) :
			mOnReached(onReachedCallback) {
		}

		inline bool HasSchedulingNode() const {
			return mNodeId.mId >= 0;
		}

		inline void Increment() {
			++mTaskCount;
		}

		inline void Increment(uint count) {
			mTaskCount += count;
		}

		inline uint ActiveTaskCount() const {
			return mTaskCount;
		}

		void OnReached(ThreadPool* pool, bool acquireMutex = true);

		inline void Decrement(ThreadPool* pool, bool acquireMutex = true) {
			uint count = mTaskCount.fetch_sub(1);
			if (count == 1) {
				OnReached(pool, acquireMutex);
			}
		}

		// Warning: not thread safe
		inline void SetCallback(TaskBarrierCallback callback) {
			mOnReached = callback;
		}

		friend class ThreadPool;
		friend class TaskQueueInterface;
		friend class TaskNodeDependencies;
	};

	class ThreadPool {
	private:
		bool bInitialized;
		std::vector<std::thread> mThreads;
		std::vector<std::queue<Task>> mIndividualImmediateQueues;
		std::thread mIOThread;
		std::unordered_map<TaskNodeId, Task> mDeferredTasks;
		std::unordered_map<TaskNodeId, ThreadPipe> mPipes;
		std::unordered_map<TaskNodeId, TaskNodeData> mNodeData;
		std::mutex mMutex;
		std::atomic<bool> bExit;
		std::queue<Task> mSharedImmediateTasks;
		std::queue<Task> mIOTasks;
		std::condition_variable mIOCondition;
		std::mutex mIOConditionMutex;
		std::mutex mIOQueueMutex;
		std::atomic<TaskNodeId> mCurrentId;
		NGraph::Graph mTaskGraph;

		void FinalizeBarrier(TaskBarrier* barrier, bool acquireMutex = true);

		TaskNodeId CreateNode(TaskNodeType type);
		void DestroyNode(TaskNodeId id);
		void TriggerOutgoing(TaskNodeId id);
		void Trigger(TaskNodeId id);
		void RepoIncomming(TaskNodeId id);
		void EnqueueTask(TaskNodeId id);

	public:
		inline uint ThreadCount() const {
			return mThreads.size() + 1;
		}

		inline uint RemainingTaskCount() const {
			return mDeferredTasks.size() + mSharedImmediateTasks.size() + mIOTasks.size();
		}

		inline uint RemainingPipeCount() const {
			return mPipes.size();
		}

		inline ThreadPool() : 
			bInitialized(false),
			bExit(false), 
			mCurrentId(0) {
		}

		inline ~ThreadPool() {
			Shutdown();
		}

		inline const entt::meta_any& ReadPipe(PipeId pipeId) {
			auto pipe = mPipes.find(pipeId.mId);
			return pipe->second.Read();
		}

		inline void WritePipe(PipeId pipeId, entt::meta_any&& data) {
			auto pipe = mPipes.find(pipeId.mId);
			pipe->second.Write(std::move(data));
		}

		inline void WritePipeException(PipeId pipeId, std::exception_ptr ex) {
			auto pipe = mPipes.find(pipeId.mId);
			pipe->second.WriteException(ex);
		}

		void ThreadProc(bool bIsMainThread, uint threadNumber, TaskBarrier* barrier, 
			const std::chrono::high_resolution_clock::time_point* quitTime);
		void IOThreadProc(uint ioThreadNumber);

		inline TaskQueueInterface GetQueue() {
			std::unique_lock<std::mutex> mutexLock(mMutex);
			std::unique_lock<std::mutex> ioMutexLock(mIOQueueMutex);

			return TaskQueueInterface(std::move(mutexLock), std::move(ioMutexLock), this);
		}

		void YieldUntilFinished();
		void YieldUntil(TaskBarrier* barrier);
		void YieldFor(const std::chrono::high_resolution_clock::duration& duration);
		void YieldUntil(const std::chrono::high_resolution_clock::time_point& time);
		void Startup(uint threads = std::thread::hardware_concurrency());
		void Shutdown();
		void WakeIO();

		friend class TaskQueueInterface;
		friend class TaskBarrier;
		friend class TaskNodeDependencies;
	};
}