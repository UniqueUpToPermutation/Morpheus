#include <Engine/ThreadPool.hpp>
#include <iostream>

namespace Morpheus {

	void Task::operator()() {
		ImmediateJobQueue queue;

		TaskParams e;
		e.mQueue = &queue;
		e.mThreadId = 0;
		if (mFunc.valid())
			mFunc(e);
	}

	void Task::operator()(const TaskParams& e) {
		if (mFunc.valid())
			mFunc(e);
	}

	void ImmediateJobQueue::Submit(Task&& taskDesc) {
		TaskParams e;
		e.mQueue = this;
		e.mThreadId = 0;

		if (taskDesc.mFunc.valid()) {
			if (taskDesc.mSyncPoint != nullptr)
				taskDesc.mSyncPoint->StartNewTask();

			taskDesc.mFunc(e);

			if (taskDesc.mSyncPoint != nullptr) 
				taskDesc.mSyncPoint->EndTask(e);
		}
	}

	void ImmediateJobQueue::YieldUntilCondition(const std::function<bool()>& predicate) {
		while (!predicate()) {
			std::this_thread::yield();
		}
	}

	void ThreadPool::ThreadProc(bool bIsMainThread, uint threadNumber, const std::function<bool()>* finishPredicate) {
		TaskParams params;
		params.mThreadId = threadNumber;
		params.mQueue = this;

		while (!bExit) {
			// We have reached our finish predicate
			if (finishPredicate && (*finishPredicate)()) {
				break;
			}

			bool bHasTask = false;

			Task task;
			{
				std::lock_guard<std::mutex> lock(mMutex);
				if (mSharedImmediateTasks.size() > 0 || mIndividualImmediateQueues[threadNumber].size() > 0) {
					bHasTask = true;

					// Take job from individual queue first
					queue_t* sourceQueue = &mSharedImmediateTasks;
					if (mIndividualImmediateQueues[threadNumber].size() > 0) {
						sourceQueue = &mIndividualImmediateQueues[threadNumber];
					}

					auto& top = sourceQueue->top();
					if ((bIsMainThread && top.mType == TaskType::FILE_IO) && mThreads.size() > 0) {
						// Main thread does not take on File IO tasks if there are other threads available to do this.
						bHasTask = false;
					}
					else {
						task = std::move(const_cast<Task&>(sourceQueue->top()));
						sourceQueue->pop();
					}
				}
			}

			if (bHasTask) {
				task.mFunc(params);

				if (task.mSyncPoint != nullptr) {
					task.mSyncPoint->EndTask(params);
				}

			} else {
				// Figure out if we should quit
				if (bIsMainThread) {
					if (finishPredicate) {
						std::this_thread::yield();
					} else {
						break;	
					}
				} else {
					std::this_thread::yield();
				}
			}
		}
	}

	void ThreadPool::YieldUntilFinished() {
		ThreadProc(true, 0, nullptr);
	}

	void ThreadPool::YieldUntilCondition(const std::function<bool()>& predicate) {
		if (!predicate()) {
			ThreadProc(true, 0, &predicate);
		}
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

		bInitialized = true;
	}

	void ThreadPool::Shutdown() {
		if (bInitialized) {
			bExit = true;

			uint i = 0;
			for (auto& thread : mThreads) {
				std::cout << "Joining Thread " << ++i << std::endl;
				thread.join();
			}

			mIndividualImmediateQueues.clear();
			mSharedImmediateTasks = queue_t();
		}
		bInitialized = false;
	}

	void ThreadPool::Submit(Task&& task) {
		if (!task.IsValid()) 
			return;

		if (task.mSyncPoint) {
			task.mSyncPoint->StartNewTask();
		}

		std::lock_guard<std::mutex> lock(mMutex);
		if (task.mAssignedThread != ASSIGN_THREAD_ANY) {
			mIndividualImmediateQueues[task.mAssignedThread].push(std::move(task));
		} else {
			if (task.mAssignedThread == ASSIGN_THREAD_MAIN && 
				task.mType == TaskType::FILE_IO &&
				mThreads.size() > 0) {
				throw std::runtime_error("Main thread does not accept file IO tasks!");
			}

			mSharedImmediateTasks.push(std::move(task));
		}
	}
}