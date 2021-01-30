#pragma once

#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <set>
#include <vector>
#include <queue>
#include <atomic>
#include <random>
#include <unordered_map>
#include <future>
#include <fstream>
#include <memory>
#include <cstdint>
#include <cstring>

#include <Engine/Graph.hpp>
#include <Engine/Entity.hpp>

#define ASSIGNED_THREAD_ANY -1

namespace Morpheus {
	typedef uint PipeId;
	typedef uint TaskId;
	typedef uint BarrierId;

	class ThreadPool;

	class TaskBarrier;

	struct TaskParams {
		ThreadPool* mPool;
		TaskBarrier* mBarrier;
		uint mThreadId;
		TaskId mTaskId;
	};

	typedef std::function<void(const TaskParams&)> TaskFunc;

	class Task {
	private:
		TaskBarrier* mBarrier;

	public:
		uint mInputsLeftToCollect;
		int mAssignedThread;
		TaskId mId;
		TaskFunc mFunc;

		inline Task(TaskFunc func, TaskId id, TaskBarrier* barrier = nullptr, 
			int assignedThread = ASSIGNED_THREAD_ANY) :
			mFunc(func),
			mId(id),
			mInputsLeftToCollect(0),
			mBarrier(barrier),
			mAssignedThread(assignedThread) {
		}

		inline Task() :
			mFunc(nullptr),
			mId(0),
			mInputsLeftToCollect(0),
			mBarrier(nullptr),
			mAssignedThread(ASSIGNED_THREAD_ANY) {
		}

		friend class ThreadPool;
	};

	class ThreadPipe {
	private:
		uint mOutputsLeftToTrigger;
		uint mInputsLeftToCollect;
		TaskId mId;
		std::promise<entt::meta_any> mPromise;
		std::shared_future<entt::meta_any> mFuture;

	public:
		inline ThreadPipe(TaskId id) :
			mOutputsLeftToTrigger(0),
			mInputsLeftToCollect(0),
			mId(id) {
			mFuture = mPromise.get_future();
		}

		inline ThreadPipe(ThreadPipe&& other) :
			mId(other.mId),
			mPromise(std::move(other.mPromise)),
			mFuture(std::move(other.mFuture)),
			mOutputsLeftToTrigger(other.mOutputsLeftToTrigger),
			mInputsLeftToCollect(other.mInputsLeftToCollect) {
		}

		inline void Write(entt::meta_any&& any) {
			mPromise.set_value(std::move(any));
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

		inline TaskQueueInterface(TaskQueueInterface&& other) {
			std::swap(mLock, other.mLock);
			std::swap(mPool, other.mPool);
			std::swap(mIOLock, other.mIOLock);
		}

		TaskId Immediate(TaskFunc func, TaskBarrier* barrier = nullptr, 
			int assignedThread = ASSIGNED_THREAD_ANY);
		TaskId Defer(TaskFunc func, TaskBarrier* barrier = nullptr, 
			int assignedThread = ASSIGNED_THREAD_ANY);

		PipeId MakePipe();

		void PipeFrom(TaskId task, PipeId pipe);
		void PipeTo(PipeId pipe, TaskId task);

		TaskId IOTask(TaskFunc func, bool bWakeIOThread = true, TaskBarrier* barrier = nullptr);
	};



	class TaskBarrier {
	private:
		std::atomic<uint> mTaskCount = 0;
		std::function<void(ThreadPool*)> mOnReached;

	public:
		inline TaskBarrier() {
		}

		inline TaskBarrier(const std::function<void(ThreadPool*)>& onReachedCallback) :
			mOnReached(onReachedCallback) {
		}

		inline void Increment() {
			++mTaskCount;
		}

		inline void Decrement() {
			--mTaskCount;
		}

		inline uint ActiveTaskCount() const {
			return mTaskCount;
		}

		inline void OnReached(ThreadPool* pool) {
			if (mOnReached)
				mOnReached(pool);
		}
	};

	class ThreadPool {
	private:
		std::vector<std::thread> mThreads;
		std::vector<std::queue<Task>> mIndividualImmediateQueues;
		std::thread mIOThread;
		std::unordered_map<TaskId, Task> mDeferredTasks;
		std::unordered_map<PipeId, ThreadPipe> mPipes;
		std::mutex mMutex;
		std::atomic<bool> bExit;
		std::queue<Task> mSharedImmediateTasks;
		std::queue<Task> mIOTasks;
		std::condition_variable mIOCondition;
		std::mutex mIOConditionMutex;
		std::mutex mIOQueueMutex;
		TaskId mCurrentId;
		NGraph::Graph mPipeGraph;

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
			bExit(false), 
			mCurrentId(0) {
		}

		inline ~ThreadPool() {
			if (!bExit) {
				Shutdown();
			}
		}

		inline const entt::meta_any& ReadPipe(PipeId pipeId) {
			auto pipe = mPipes.find(pipeId);
			return pipe->second.Read();
		}

		inline void WritePipe(PipeId pipeId, entt::meta_any&& data) {
			auto pipe = mPipes.find(pipeId);
			pipe->second.Write(std::move(data));
		}

		void ThreadProc(bool bIsMainThread, uint threadNumber, TaskBarrier* barrier);
		void IOThreadProc(uint ioThreadNumber);

		inline TaskQueueInterface GetQueue() {
			return TaskQueueInterface(std::unique_lock<std::mutex>(mMutex),
				std::unique_lock<std::mutex>(mIOQueueMutex),
				this);
		}

		void Yield();
		void YieldUntil(TaskBarrier* barrier);
		void Startup(uint threads = std::thread::hardware_concurrency());
		void Shutdown();
		void WakeIO();

		friend class TaskQueueInterface;
	};
}