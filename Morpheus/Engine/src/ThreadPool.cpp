#include <Engine/ThreadPool.hpp>

namespace Morpheus {
	TaskId TaskQueueInterface::Immediate(TaskFunc func, TaskBarrier* barrier, int assignedThread) {
		if (barrier)
			barrier->Increment();

		TaskId id = mPool->mCurrentId++;
		Task task(func, id, barrier, assignedThread);

		if (assignedThread == ASSIGNED_THREAD_ANY) {
			mPool->mSharedImmediateTasks.push(task);
		} else {
			mPool->mIndividualImmediateQueues[assignedThread].push(task);
		}

		mPool->mPipeGraph.insert_vertex(id);
		return id;
	}

	TaskId TaskQueueInterface::Defer(TaskFunc func, TaskBarrier* barrier, int assignedThread) {
		if (barrier)
			barrier->Increment();

		TaskId id = mPool->mCurrentId++;
		Task task(func, id, barrier, assignedThread);
		mPool->mDeferredTasks[id] = task;
		mPool->mPipeGraph.insert_vertex(id);
		return id;
	}

	PipeId TaskQueueInterface::MakePipe() {
		PipeId id = mPool->mCurrentId++;
		mPool->mPipes.emplace(id, ThreadPipe(id));
		mPool->mPipeGraph.insert_vertex(id);
		return id;
	}

	void TaskQueueInterface::PipeFrom(TaskId task, PipeId pipe) {
		mPool->mPipeGraph.insert_edge(task, pipe);

		auto pipeIt = mPool->mPipes.find(pipe);
		pipeIt->second.mInputsLeftToCollect++;
	}

	void TaskQueueInterface::PipeTo(PipeId pipe, TaskId task) {
		mPool->mPipeGraph.insert_edge(pipe, task);
		auto pipeIt = mPool->mPipes.find(pipe);
		auto taskIt = mPool->mDeferredTasks.find(task);

		taskIt->second.mInputsLeftToCollect++;
		pipeIt->second.mOutputsLeftToTrigger++;
	}

	TaskId TaskQueueInterface::IOTask(TaskFunc func, bool bWakeIO, TaskBarrier* barrier) {
		if (barrier)
			barrier->Increment();

		TaskId id = mPool->mCurrentId++;
		Task task(func, id, barrier);

		mPool->mIOTasks.push(task);
		mPool->mPipeGraph.insert_vertex(id);

		if (bWakeIO)
			mPool->WakeIO();

		return id;
	}

	void ThreadPool::ThreadProc(bool bIsMainThread, uint threadNumber, TaskBarrier* barrier) {
		TaskParams params;
		params.mThreadId = threadNumber;
		params.mPool = this;

		std::vector<uint> pipesToDestroy;
		std::vector<uint> pipesToTrigger;

		while (!bExit) {
			// We have reached our desired barrier
			if (barrier && barrier->ActiveTaskCount() == 0) {
				break;
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

					// Round up all incomming pipes
					auto incommingPipes = mPipeGraph.in_neighbors(task.mId);
					for (auto pipeIt = incommingPipes.begin(); pipeIt != incommingPipes.end(); ++pipeIt) {
						auto pipeDictIt = mPipes.find(*pipeIt);
						--pipeDictIt->second.mOutputsLeftToTrigger;

						// Destroy pipes that have no outputs left to trigger
						if (pipeDictIt->second.mOutputsLeftToTrigger == 0) {
							pipesToDestroy.emplace_back(*pipeIt);
						}
					}

					// Round up all outgoing pipes
					auto outgoingPipes = mPipeGraph.out_neighbors(task.mId);
					for (auto pipeIt = outgoingPipes.begin(); pipeIt != outgoingPipes.end(); ++pipeIt) {
						auto pipeDictIt = mPipes.find(*pipeIt);
						--pipeDictIt->second.mInputsLeftToCollect;

						// Trigger pipes that have no inputs left to collect
						if (pipeDictIt->second.mInputsLeftToCollect == 0) {
							pipesToTrigger.emplace_back(*pipeIt);
						}						
					}

					// Remove this task vertex from the graph
					mPipeGraph.remove_vertex(task.mId);
				}
			}

			if (bHasTask) {
				params.mBarrier = task.mBarrier;
				params.mTaskId = task.mId;
				task.mFunc(params);

				// Need to update pipes
				if (pipesToTrigger.size() > 0 || pipesToDestroy.size() > 0)
				{
					std::lock_guard<std::mutex> lock(mMutex);
					for (auto pipe : pipesToTrigger) {
						auto outgoingTasks = mPipeGraph.out_neighbors(pipe);

						for (auto it = outgoingTasks.begin(); it != outgoingTasks.end(); ++it) {
							// If this is the only pipe that the task is waiting on 
							auto taskIt = mDeferredTasks.find(*it);
							--taskIt->second.mInputsLeftToCollect;

							// Trigger the task if all input pipes are ready
							if (task.mInputsLeftToCollect == 0) {
								if (taskIt->second.mAssignedThread == ASSIGNED_THREAD_ANY) {
									mSharedImmediateTasks.push(taskIt->second);
								} else {
									mIndividualImmediateQueues[taskIt->second.mAssignedThread].push(taskIt->second);
								}
								mDeferredTasks.erase(taskIt);
							}
						}
					}

					for (auto pipeId : pipesToDestroy) {
						auto pipe = mPipes.find(pipeId);
						mPipes.erase(pipe);
					}

					pipesToDestroy.clear();
					pipesToTrigger.clear();
				}

				// Release the barrier object
				if (params.mBarrier) {
					params.mBarrier->Decrement();

					// If there are no tasks left for this barrier, invoke the OnReached callback
					if (params.mBarrier->ActiveTaskCount() == 0) {
						params.mBarrier->OnReached(this);
					}
				}
			} else {
				// IO tasks queued?
				if (mIOTasks.size() > 0) {
					std::unique_lock<std::mutex> lock(mIOConditionMutex, std::try_to_lock);
					if (lock.owns_lock()){
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
		std::vector<uint> pipesToTrigger;

		Task task;
		bool bHasTask = false;

		while (!bExit) {
			{
				std::lock_guard<std::mutex> queueLock(mIOQueueMutex);
				bHasTask = mIOTasks.size() > 0;
				if (bHasTask) {
					task = mIOTasks.front();
					mIOTasks.pop();

					std::lock_guard<std::mutex> mainLock(mMutex);

					// Round up all outgoing pipes
					auto outgoingPipes = mPipeGraph.out_neighbors(task.mId);
					for (auto pipeIt = outgoingPipes.begin(); pipeIt != outgoingPipes.end(); ++pipeIt) {
						pipesToTrigger.emplace_back(*pipeIt);
					}

					// Remove this task vertex from the graph
					mPipeGraph.remove_vertex(task.mId);
				}
			}

			if (bHasTask) {
				params.mTaskId = task.mId;
				task.mFunc(params);

				// Need to update pipes
				if (pipesToTrigger.size() > 0)
				{
					std::lock_guard<std::mutex> mainLock(mMutex);
					for (auto pipe : pipesToTrigger) {
						auto outgoingTasks = mPipeGraph.out_neighbors(pipe);

						for (auto it = outgoingTasks.begin(); it != outgoingTasks.end(); ++it) {
							// If this is the only pipe that the task is waiting on 
							auto taskIt = mDeferredTasks.find(*it);
							--taskIt->second.mInputsLeftToCollect;

							// Trigger the task if all input pipes are ready
							if (taskIt->second.mInputsLeftToCollect == 0) {

								if (taskIt->second.mAssignedThread == ASSIGNED_THREAD_ANY) {
									mSharedImmediateTasks.push(taskIt->second);
								} else {
									mIndividualImmediateQueues[taskIt->second.mAssignedThread].push(taskIt->second);
								}
								mDeferredTasks.erase(taskIt);
							}
						}
					}
					pipesToTrigger.clear();
				}

				// Release the barrier object
				if (task.mBarrier) {
					task.mBarrier->Decrement();

					// Trigger the callback if the barrier has been reached
					if (task.mBarrier->ActiveTaskCount() == 0) {
						task.mBarrier->OnReached(this);
					}
				}
			}
			else {
				mIOCondition.wait(lock);
			}
		}
	}

	void ThreadPool::Yield() {
		ThreadProc(true, 0, nullptr);
	}

	void ThreadPool::YieldUntil(TaskBarrier* barrier) {
		ThreadProc(true, 0, barrier);
	}

	void ThreadPool::Startup(uint threads) {	
		bExit = false;

		mIndividualImmediateQueues.resize(threads);

		for (uint i = 1; i < threads; ++i) {
			std::cout << "Initializing Thread " << i << std::endl;
			std::function<void()> threadProc = [this, i]() {
				ThreadProc(false, i, nullptr);
			};
			mThreads.emplace_back(std::thread(threadProc));
		}

		std::cout << "Initializing IO Thread" << std::endl;
		mIOThread = std::thread([this]() {
			IOThreadProc(0);
		});
	}

	void ThreadPool::Shutdown() {
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

	void ThreadPool::WakeIO() {
		mIOCondition.notify_one();
	}
}