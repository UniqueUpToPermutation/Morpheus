#include <Engine/ThreadPool.hpp>

namespace Morpheus {
	void TaskBarrier::OnReached(ThreadPool* pool, bool acquireMutex) {
		if (mOnReached) {
			auto tmp = mOnReached;
			mOnReached = nullptr;
			tmp(pool);
		}

		pool->FinalizeBarrier(this, acquireMutex);
	}

	PipeId TaskQueueInterface::MakePipe() {
		TaskNodeId id = mPool->CreateNode(TaskNodeType::PIPE_NODE);
		mPool->mPipes.emplace(id, ThreadPipe(id));
		return id;
	}

	TaskId TaskQueueInterface::MakeTask(const TaskDesc& desc) {
		TaskNodeId id = mPool->CreateNode(TaskNodeType::TASK_NODE);
		mPool->mDeferredTasks.emplace(id, Task(desc, id));

		if (desc.mBarrier) {
			desc.mBarrier->Increment();
		}

		return id;
	}

	TaskNodeDependencies TaskQueueInterface::Dependencies(TaskId task) {
		return TaskNodeDependencies(task.mId, mPool, mPool->mNodeData.find(task.mId));
	}

	TaskNodeDependencies TaskQueueInterface::Dependencies(PipeId pipe) {
		return TaskNodeDependencies(pipe.mId, mPool, mPool->mNodeData.find(pipe.mId));
	}

	void TaskQueueInterface::Schedule(TaskId task) {
		if (task.IsValid()) {
			auto data = mPool->mNodeData.find(task.mId);

			if (data->second.mInputsLeftToCollect == 0) {
				mPool->EnqueueTask(task.mId);
			}
		}
	}

	void TaskNodeDependencies::After(TaskNodeId otherId) {
		auto other = mPool->mNodeData.find(otherId);

		mIterator->second.mInputsLeftToCollect++;
		other->second.mOutputsLeftToTrigger++;

		mPool->mTaskGraph.insert_edge(otherId, mId);
	}

	TaskNodeDependencies& TaskNodeDependencies::After(TaskBarrier* barrier) {
		if (barrier->ActiveTaskCount() > 0) {
			if (!barrier->HasSchedulingNode()) {
				barrier->mNodeId = mPool->CreateNode(TaskNodeType::BARRIER_NODE);
			}

			auto other = mPool->mNodeData.find(barrier->mNodeId.mId);

			mIterator->second.mInputsLeftToCollect++;
			other->second.mOutputsLeftToTrigger++;

			mPool->mTaskGraph.insert_edge(barrier->mNodeId.mId, mId);
		}

		return *this;
	}

	void ThreadPool::ThreadProc(bool bIsMainThread, uint threadNumber, TaskBarrier* barrier,
		const std::chrono::high_resolution_clock::time_point* quitTime) {
		TaskParams params;
		params.mThreadId = threadNumber;
		params.mPool = this;

		while (!bExit) {
			// We have reached our desired barrier
			if (barrier && barrier->ActiveTaskCount() == 0) {
				break;
			}

			// We have reached the maximum yield time
			if (quitTime) {
				auto time = std::chrono::high_resolution_clock::now();
				if (time > *quitTime) {
					break;
				}
			}

			bool bHasTask = false;

			Task task;
			{
				std::lock_guard<std::mutex> lock(mMutex);
				if (mSharedImmediateTasks.size() > 0 || mIndividualImmediateQueues[threadNumber].size() > 0) {
					bHasTask = true;

					// Take job from individual queue first
					std::queue<Task>* sourceQueue = &mSharedImmediateTasks;
					if (mIndividualImmediateQueues[threadNumber].size() > 0) {
						sourceQueue = &mIndividualImmediateQueues[threadNumber];
					}

					task = sourceQueue->front();
					sourceQueue->pop();
				}
			}

			if (bHasTask) {
				params.mBarrier = task.mBarrier;
				params.mTaskId = task.mId;
				task.mFunc(params);

				{
					std::lock_guard<std::mutex> lock(mMutex);

					RepoIncomming(task.mId);
					TriggerOutgoing(task.mId);
					DestroyNode(task.mId);
				}

				// Release the barrier object
				if (params.mBarrier) {
					// If there are no tasks left for this barrier, invoke the OnReached callback
					params.mBarrier->Decrement(this, true);
				}
			} else {
				// IO tasks queued?
				if (mIOTasks.size() > 0) {
					std::unique_lock<std::mutex> lock(mIOConditionMutex, std::try_to_lock);
					if (lock.owns_lock()) {
						mIOCondition.notify_one(); // We need to wake IO thread
					}
				}

				// Figure out if we should quit
				if (bIsMainThread) {
					if (mDeferredTasks.size() == 0) {
						if (barrier) {
							std::this_thread::yield();
						} else {
							break;	
						}
					}
				} else {
					std::this_thread::yield();
				}
			}
		}
	}

	void ThreadPool::IOThreadProc(uint ioThreadNumber) {
		TaskParams params;
		params.mPool = this;
		params.mThreadId = ioThreadNumber;

		std::unique_lock<std::mutex> lock(mIOConditionMutex);

		Task task;
		bool bHasTask = false;

		while (!bExit) {
			{
				std::lock_guard<std::mutex> queueLock(mIOQueueMutex);
				bHasTask = mIOTasks.size() > 0;
				if (bHasTask) {
					task = mIOTasks.front();
					mIOTasks.pop();
				}
			}

			if (bHasTask) {
				params.mTaskId = task.mId;
				task.mFunc(params);

				// Need to update pipes
				{
					std::lock_guard<std::mutex> pipeLock(mMutex);
					RepoIncomming(params.mTaskId);
					TriggerOutgoing(params.mTaskId);
					DestroyNode(params.mTaskId);
				}

				// Release the barrier object
				if (task.mBarrier) {
					// If there are no tasks left for this barrier, invoke the OnReached callback
					params.mBarrier->Decrement(this, true);
				}
			}
			else {
				mIOCondition.wait(lock);
			}
		}
	}

	void ThreadPool::YieldUntilFinished() {
		ThreadProc(true, 0, nullptr, nullptr);
	}

	TaskNodeId ThreadPool::CreateNode(TaskNodeType type) {
		++mCurrentId;

		if (mCurrentId < 0) { // Handle overflow
			mCurrentId = 0;
		}

		TaskNodeId id = mCurrentId;

		mTaskGraph.insert_vertex(id);
		mNodeData[id] = TaskNodeData(type);

		return id;
	}

	void ThreadPool::DestroyNode(TaskNodeId id) {
		mTaskGraph.remove_vertex(id);
		auto data = mNodeData.find(id);

		switch (data->second.mType) {
		case TaskNodeType::BARRIER_NODE:
			break;

		case TaskNodeType::PIPE_NODE:
			mPipes.erase(id);
			break;

		case TaskNodeType::TASK_NODE:
			break;

		default:
			throw std::runtime_error("Unknown task node type!");
		}
	}

	void ThreadPool::TriggerOutgoing(TaskNodeId id) {
		auto outgoingTasks = mTaskGraph.out_neighbors(id);

		for (auto outTask = outgoingTasks.begin(); outTask != outgoingTasks.end(); ++outTask) {
			Trigger(*outTask);
		}
	}

	void ThreadPool::RepoIncomming(TaskNodeId id) {
		auto incommingTasks = mTaskGraph.in_neighbors(id);

		for (auto inTask = incommingTasks.begin(); inTask != incommingTasks.end(); ++inTask) {
			auto data = mNodeData.find(*inTask);

			--data->second.mOutputsLeftToTrigger;

			if (data->second.mOutputsLeftToTrigger == 0) {
				DestroyNode(*inTask);
			}
		}
	}

	void ThreadPool::Trigger(TaskNodeId id) {
		auto data = mNodeData.find(id);

		--data->second.mInputsLeftToCollect;

		if (data->second.mInputsLeftToCollect == 0) {
			switch (data->second.mType) {
				case TaskNodeType::PIPE_NODE:
					TriggerOutgoing(id);
					break;
				case TaskNodeType::TASK_NODE:
					EnqueueTask(id);
					break;
				default:
					throw std::runtime_error("Unacceptable TaskNodeType detected!");
			}
		}
	}

	void ThreadPool::EnqueueTask(TaskNodeId id) {
		auto taskIt = mDeferredTasks.find(id);
		if (taskIt->second.mAssignedThread == ASSIGN_THREAD_ANY) {
			mSharedImmediateTasks.push(taskIt->second);
		} else if (taskIt->second.mAssignedThread == ASSIGN_THREAD_IO) {
			mIOTasks.emplace(taskIt->second);
			WakeIO();
		} else {
			mIndividualImmediateQueues[taskIt->second.mAssignedThread].push(taskIt->second);
		}
		mDeferredTasks.erase(taskIt);
	}

	void ThreadPool::FinalizeBarrier(TaskBarrier* barrier, bool acquireMutex) {
		std::unique_lock<std::mutex> lock;

		if (acquireMutex) {
			lock = std::unique_lock<std::mutex>(mMutex);
		}

		if (barrier->HasSchedulingNode()) {
			// Barrier has children, attempt to trigger all children
			TriggerOutgoing(barrier->mNodeId.mId);
			DestroyNode(barrier->mNodeId.mId);
			barrier->mNodeId = -1;
		}
	}

	void ThreadPool::YieldUntil(TaskBarrier* barrier) {
		ThreadProc(true, 0, barrier, nullptr);
	}

	void ThreadPool::YieldFor(const std::chrono::high_resolution_clock::duration& duration) {
		std::chrono::high_resolution_clock::time_point time = std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::time_point stopTime = time + duration;

		ThreadProc(true, 0, nullptr, &stopTime);
	}

	void ThreadPool::YieldUntil(const std::chrono::high_resolution_clock::time_point& time) {
		ThreadProc(true, 0, nullptr, &time);
	}

	void ThreadPool::Startup(uint threads) {	
		bExit = false;

		mIndividualImmediateQueues.resize(threads);

		for (uint i = 1; i < threads; ++i) {
			std::cout << "Initializing Thread " << i << std::endl;
			std::function<void()> threadProc = [this, i]() {
				ThreadProc(false, i, nullptr, nullptr);
			};
			mThreads.emplace_back(std::thread(threadProc));
		}

		std::cout << "Initializing IO Thread" << std::endl;
		mIOThread = std::thread([this]() {
			IOThreadProc(0);
		});

		bInitialized = true;
	}

	void ThreadPool::Shutdown() {
		if (bInitialized) {
			bExit = true;

			mIOCondition.notify_one();

			std::cout << "Joining IO Thread" << std::endl;
			mIOThread.join();

			uint i = 0;
			for (auto& thread : mThreads) {
				std::cout << "Joining Thread " << ++i << std::endl;
				thread.join();
			}

			mPipes.clear();
			mIndividualImmediateQueues.clear();
		}
		bInitialized = false;
	}

	void ThreadPool::WakeIO() {
		mIOCondition.notify_one();
	}
}