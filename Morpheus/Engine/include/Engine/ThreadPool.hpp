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

#include <Engine/Graph.hpp>
#include <Engine/Entity.hpp>

#define ASSIGN_THREAD_ANY -1
#define ASSIGN_THREAD_MAIN 0

#define TASK_NONE -1 
#define PIPE_NONE -1
#define BARRIER_NONE -1

namespace Morpheus {
	typedef int PipeId;
	typedef int TaskId;
	typedef int BarrierId;

	class ThreadPool;
	class TaskBarrier;

	struct TaskParams {
		ThreadPool* mPool;
		TaskBarrier* mBarrier;
		uint mThreadId;
		TaskId mTaskId;
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

	class Task {
	private:
		TaskBarrier* mBarrier;

	public:
		int mAssignedThread;
		TaskId mId;
		TaskFunc mFunc;

		inline Task(TaskFunc func, TaskId id, TaskBarrier* barrier = nullptr, 
			int assignedThread = ASSIGN_THREAD_ANY) :
			mFunc(func),
			mId(id),
			mBarrier(barrier),
			mAssignedThread(assignedThread) {
		}

		inline Task(const TaskDesc& task, TaskId id) :
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
		TaskId mId;
		std::promise<entt::meta_any> mPromise;
		std::shared_future<entt::meta_any> mFuture;

	public:
		inline ThreadPipe(TaskId id) :
			mId(id) {
			mFuture = mPromise.get_future();
		}

		inline ThreadPipe(ThreadPipe&& other) :
			mId(other.mId),
			mPromise(std::move(other.mPromise)),
			mFuture(std::move(other.mFuture)) {
		}

		inline void Write(entt::meta_any&& any) {
			mPromise.set_value(std::move(any));
		}

		inline void WriteException(std::__exception_ptr::exception_ptr ex) {
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

		inline TaskQueueInterface(TaskQueueInterface&& other) {
			std::swap(mLock, other.mLock);
			std::swap(mPool, other.mPool);
			std::swap(mIOLock, other.mIOLock);
		}

		// Queues a task for immediate execution
		TaskId Immediate(const TaskDesc& task);
		// Converts a deferred task into an immediate task
		void MakeImmediate(TaskId task);
		// Queues a task for immediate execution
		inline TaskId Immediate(TaskFunc func, TaskBarrier* barrier = nullptr, 
			int assignedThread = ASSIGN_THREAD_ANY) {
			return Immediate(TaskDesc(func, barrier, assignedThread));
		}

		TaskId Defer(const TaskDesc& task);
		inline TaskId Defer(TaskFunc func, TaskBarrier* barrier = nullptr, 
			int assignedThread = ASSIGN_THREAD_ANY) {
			return Defer(TaskDesc(func, barrier, assignedThread));
		}

		PipeId MakePipe();

		void PipeFrom(TaskId task, PipeId pipe);
		void PipeTo(PipeId pipe, TaskId task);
		void ScheduleAfter(TaskBarrier* before, TaskId after);

		TaskId IOTask(TaskFunc func, bool bWakeIOThread = true, TaskBarrier* barrier = nullptr);
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
			return mNodeId >= 0;
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

		inline void OnReached(ThreadPool* pool) {
			if (mOnReached) {
				auto tmp = mOnReached;
				mOnReached = nullptr;
				tmp(pool);
			}
		}

		inline void Decrement(ThreadPool* pool, bool bTriggerOnReached = true) {
			uint count = mTaskCount.fetch_sub(1);
			if (bTriggerOnReached && count == 1) {
				OnReached(pool);
			}
		}

		// Warning: not thread safe
		inline void SetCallback(TaskBarrierCallback callback) {
			mOnReached = callback;
		}

		friend class ThreadPool;
		friend class TaskQueueInterface;
	};

	enum class TaskNodeType {
		UNKNOWN_NODE,
		TASK_NODE,
		PIPE_NODE,
		BARRIER_NODE
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

	class ThreadPool {
	private:
		bool bInitialized;
		std::vector<std::thread> mThreads;
		std::vector<std::queue<Task>> mIndividualImmediateQueues;
		std::thread mIOThread;
		std::unordered_map<TaskId, Task> mDeferredTasks;
		std::unordered_map<PipeId, ThreadPipe> mPipes;
		std::unordered_map<uint, TaskNodeData> mNodeData;
		std::mutex mMutex;
		std::atomic<bool> bExit;
		std::queue<Task> mSharedImmediateTasks;
		std::queue<Task> mIOTasks;
		std::condition_variable mIOCondition;
		std::mutex mIOConditionMutex;
		std::mutex mIOQueueMutex;
		std::atomic<TaskId> mCurrentId;
		NGraph::Graph mTaskGraph;

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
			auto pipe = mPipes.find(pipeId);
			return pipe->second.Read();
		}

		inline void WritePipe(PipeId pipeId, entt::meta_any&& data) {
			auto pipe = mPipes.find(pipeId);
			pipe->second.Write(std::move(data));
		}

		inline void WritePipeException(PipeId pipeId, std::__exception_ptr::exception_ptr ex) {
			auto pipe = mPipes.find(pipeId);
			pipe->second.WriteException(ex);
		}

		void ThreadProc(bool bIsMainThread, uint threadNumber, TaskBarrier* barrier, 
			const std::chrono::high_resolution_clock::time_point* quitTime);
		void IOThreadProc(uint ioThreadNumber);

		inline TaskQueueInterface GetQueue() {
			return TaskQueueInterface(std::unique_lock<std::mutex>(mMutex),
				std::unique_lock<std::mutex>(mIOQueueMutex),
				this);
		}

		void Yield();
		void YieldUntil(TaskBarrier* barrier);
		void YieldFor(const std::chrono::high_resolution_clock::duration& duration);
		void YieldUntil(const std::chrono::high_resolution_clock::time_point& time);
		void Startup(uint threads = std::thread::hardware_concurrency());
		void Shutdown();
		void WakeIO();

		friend class TaskQueueInterface;
	};
}