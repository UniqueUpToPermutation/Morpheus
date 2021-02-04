#include <Engine/ThreadPool.hpp>

namespace Morpheus {
	TaskId TaskQueueInterface::Immediate(const TaskDesc& desc) {
		if (desc.mBarrier)
			desc.mBarrier->Increment();

		mPool->mCurrentId++;
		if (mPool->mCurrentId < 0) {
			mPool->mCurrentId = 0;
		}
		TaskId id = mPool->mCurrentId;

		Task task(desc, id);

		if (task.mAssignedThread == ASSIGN_THREAD_ANY) {
			mPool->mSharedImmediateTasks.push(task);
		} else {
			mPool->mIndividualImmediateQueues[task.mAssignedThread].push(task);
		}

		mPool->mPipeGraph.insert_vertex(id);
		return id;
	}

	void TaskQueueInterface::MakeImmediate(TaskId task) {
		// If this is the only pipe that the task is waiting on 
		auto taskIt = mPool->mDeferredTasks.find(task);
		assert(taskIt->second.mInputsLeftToCollect == 0);

		// Trigger the task if all input pipes are ready
		if (taskIt->second.mAssignedThread == ASSIGN_THREAD_ANY) {
			mPool->mSharedImmediateTasks.push(taskIt->second);
		} else {
			mPool->mIndividualImmediateQueues[taskIt->second.mAssignedThread].push(taskIt->second);
		}
		mPool->mDeferredTasks.erase(taskIt);
	}

	TaskId TaskQueueInterface::Defer(const TaskDesc& desc) {
		if (desc.mBarrier)
			desc.mBarrier->Increment();

		mPool->mCurrentId++;
		if (mPool->mCurrentId < 0) { // Handle overflow
			mPool->mCurrentId = 0;
		}
		TaskId id = mPool->mCurrentId;

		Task task(desc, id);
		mPool->mDeferredTasks[id] = task;
		mPool->mPipeGraph.insert_vertex(id);
		return id;
	}

	PipeId TaskQueueInterface::MakePipe() {
		mPool->mCurrentId++;
		if (mPool->mCurrentId < 0) { // Handle overflow
			mPool->mCurrentId = 0;
		}
		PipeId id = mPool->mCurrentId;

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

		mPool->mCurrentId++;
		if (mPool->mCurrentId < 0) { // Handle overflow
			mPool->mCurrentId = 0;
		}
		TaskId id = mPool->mCurrentId;

		Task task(func, id, barrier);

		mPool->mIOTasks.push(task);
		mPool->mPipeGraph.insert_vertex(id);

		if (bWakeIO)
			mPool->WakeIO();

		return id;
	}

	void ThreadPool::ThreadProc(bool bIsMainThread, uint threadNumber, TaskBarrier* barrier,
		const std::chrono::high_resolution_clock::time_point* quitTime) {
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
								if (taskIt->second.mAssignedThread == ASSIGN_THREAD_ANY) {
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
					// If there are no tasks left for this barrier, invoke the OnReached callback
					params.mBarrier->Decrement(this);
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

								if (taskIt->second.mAssignedThread == ASSIGN_THREAD_ANY) {
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
					// Trigger the callback if the barrier has been reached
					task.mBarrier->Decrement(this);
				}
			}
			else {
				mIOCondition.wait(lock);
			}
		}
	}

	void ThreadPool::Yield() {
		ThreadProc(true, 0, nullptr, nullptr);
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