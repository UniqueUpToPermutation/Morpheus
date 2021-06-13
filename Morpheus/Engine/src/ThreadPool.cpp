#include <Engine/ThreadPool.hpp>
#include <iostream>

#ifdef THREAD_POOL_DEBUG
std::mutex gPoolOutput;
#endif

namespace Morpheus {

	void TaskBarrier::Trigger() {
		auto lock = mOut.Lock();

		if (!lock.IsFinished()) {
			ImmediateTaskQueue queue;
			queue.Trigger(this);
		}
	}

	ITaskQueue::~ITaskQueue() {
	}

	TaskNodeInLock& TaskNodeInLock::Connect(TaskNodeOut* out) {
		if (out) {
			auto lock = out->Lock();

			if (!lock.IsFinished()) {
				out->mOutputs.push_back(mNode);
				mNode->mInputs.push_back(out);
				mNode->mInputsLeft++;
				bWait = true;
			}
		}

		return *this;
	}

	void TaskNodeIn::ResetUnsafe() {
		mInputsLeft = mInputs.size();
		bIsStarted = false;
	}

	void TaskNodeOut::ResetUnsafe() {
		bIsFinished = false;
	}

	void ImmediateTaskQueue::Emplace(ITask* task) {
		mImmediateQueue.push(task);
	}

	void ImmediateTaskQueue::YieldUntilCondition(const std::function<bool()>& predicate) {
		while (!predicate()) {
			RunOneJob();
		}
	}

	void ImmediateTaskQueue::YieldUntilEmpty() {
		YieldUntilCondition([this]{
			return RemainingJobCount() == 0;
		});
	}

	ITask* ImmediateTaskQueue::Adopt(Task&& task) {
		auto ptr = task.Ptr();
	
		mOwnedTasks.emplace(ptr, std::move(task));

		return ptr;
	}

	void ImmediateTaskQueue::RunOneJob() {
		if (mImmediateQueue.size() == 0)
			return;

		Task task = std::move(mImmediateQueue.front());
		mImmediateQueue.pop();

		// Signal that we've started this task!
		{
			auto lock = task->In().Lock();
			
#ifdef THREAD_POOL_DEBUG
			if (!task->In().bIsStarted) {
				std::lock_guard<std::mutex> lock2(gPoolOutput);
				std::cout << "Start Task: " 
					<< task->GetName() << std::endl;
			} else {
				std::lock_guard<std::mutex> lock2(gPoolOutput);
				std::cout << "Task Unshelved: " 
					<< task->GetName() << std::endl;
			}
#endif

			task->In().bIsStarted = true;
		}

		TaskParams params;
		params.mQueue = this;
		params.mThreadId = ASSIGN_THREAD_MAIN;
		params.mTask = task.Ptr();

		auto result = task->Run(params);

		if (result != TaskResult::FINISHED) {

			if (result == TaskResult::WAITING) {
#ifdef THREAD_POOL_DEBUG
				{
					std::lock_guard<std::mutex> lock(gPoolOutput);
					std::cout << "Shelving Task: " 
						<< task->GetName() << std::endl;
				}
#endif
			}
			else if (result == TaskResult::REQUEST_THREAD_SWITCH) {
				throw std::runtime_error("Thread switch not supported with Immediate Queue!");
			}

		} else {
#ifdef THREAD_POOL_DEBUG
			{
				std::lock_guard<std::mutex> lock(gPoolOutput);
				std::cout << "End Task: " 
					<< task->GetName() << std::endl;
			}
#endif

			Fire(&task->Out());

			auto it = mOwnedTasks.find(task.Ptr());
			if (it != mOwnedTasks.end()) {
				mOwnedTasks.erase(it);
			}
		}
	}

	void ITaskQueue::Trigger(TaskNodeIn* in, bool bFromNodeOut) {
		bool bActuallyTrigger = false;
		{
			std::lock_guard<std::mutex> lock(in->mMutex);
			if (bFromNodeOut) {
				in->mInputsLeft--;
				bActuallyTrigger = in->mInputsLeft == 0;
			} else {
				bActuallyTrigger = in->mInputsLeft == 0;
			}
		}

		if (bActuallyTrigger) {
			switch (in->mOwnerType) {
			case TaskPinOwnerType::BARRIER:
#ifdef THREAD_POOL_DEBUG
				{
					std::lock_guard<std::mutex> lock2(gPoolOutput);
					std::cout << "Barrier Triggered!" << std::endl;
				}
#endif
				Fire(&in->mOwner.mBarrier->mOut);
				break;
			case TaskPinOwnerType::TASK:
				Emplace(in->mOwner.mTask);
				break;
			}
		}
	}

	void ITaskQueue::Fire(TaskNodeOut* out) {
		std::lock_guard<std::mutex> lock(out->mMutex);
		out->bIsFinished = true;

		for (auto in : out->mOutputs) {
			Trigger(in, true);
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

			ITask* task = nullptr;
			{
				// Select a task assigned to the current thread
				{
					std::lock_guard<std::mutex> lock(mIndividualMutexes[threadNumber]);
					if (mIndividualQueues[threadNumber].size() > 0) {
						task = mIndividualQueues[threadNumber].top();
						mIndividualQueues[threadNumber].pop();
					}
				}

				if (!task) {

					// Select a task assigned to the collective queue
					std::lock_guard<std::mutex> lock(mCollectiveQueueMutex);

					while (mCollectiveQueue.size() > 0) {
						task = mCollectiveQueue.top();
						mCollectiveQueue.pop();

						auto assignedThread = task->GetAssignedThread();

						if (assignedThread != ASSIGN_THREAD_ANY &&
							assignedThread != threadNumber) {
							// This task was meant for another thread, assign it to that thread
							Emplace(task);
							--mTasksPending;
						} else {
							// This task can be performed by this thread.
							break;
						}
					}
				}
			}

			if (task) {

				// Start this task if it hasn't already
				{
					auto lock = task->In().Lock();
					
#ifdef THREAD_POOL_DEBUG
					if (!task->In().bIsStarted) {
						std::lock_guard<std::mutex> lock2(gPoolOutput);
						std::cout << "Start Task (Thread " << threadNumber << "): " 
							<< task->GetName() << std::endl;
					} else {
						std::lock_guard<std::mutex> lock2(gPoolOutput);
						std::cout << "Task Unshelved (Thread " << threadNumber << "): " 
							<< task->GetName() << std::endl;
					}
#endif
					if (task->In().mInputsLeft != 0) {

#ifdef THREAD_POOL_DEBUG
						{
							std::lock_guard<std::mutex> lock(gPoolOutput);
							std::cout << "Shelving Task (Thread " << threadNumber << "): " 
								<< task->GetName() << std::endl;
						}
#endif
					} else {
						task->In().bIsStarted = true;
					}
				}

				params.mTask = task;
				auto result = task->Run(params);

				if (result == TaskResult::FINISHED) {
#ifdef THREAD_POOL_DEBUG
					{
						std::lock_guard<std::mutex> lock(gPoolOutput);
						std::cout << "End Task (Thread " << threadNumber << "): " 
							<< task->GetName() << std::endl;
					}
#endif
					// Finish Task
					Fire(&task->Out());

					{
						// If we own the task, we should deallocate it
						std::lock_guard<std::mutex> lock(mOwnedTasksMutex);
						auto it = mOwnedTasks.find(task);
						if (it != mOwnedTasks.end()) {
							mOwnedTasks.erase(it);
						}
					}

					--mTasksPending;
				} else {
					if (result == TaskResult::WAITING) {

#ifdef THREAD_POOL_DEBUG
						{
							std::lock_guard<std::mutex> lock(gPoolOutput);
							std::cout << "Shelving Task (Thread " << threadNumber << "): " 
								<< task->GetName() << std::endl;
						}
#endif

					} else if (result == TaskResult::REQUEST_THREAD_SWITCH) {
						auto assignedThread = task->GetAssignedThread();

#ifdef THREAD_POOL_DEBUG
						{
							std::lock_guard<std::mutex> lock(gPoolOutput);
							std::cout << "Swapping Thread (" << threadNumber << "->" 
								<< assignedThread << "): " 
								<< task->GetName() << std::endl;
						}
#endif
						// This task was meant for another thread, assign it to that thread
						Emplace(task);
						--mTasksPending;
					}
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

	void ThreadPool::Emplace(ITask* task) {
		++mTasksPending;
		auto thread = task->GetAssignedThread();

		if (thread == ASSIGN_THREAD_ANY) {
			std::lock_guard<std::mutex> lock(mCollectiveQueueMutex);
			mCollectiveQueue.emplace(task);
		} else {
			std::lock_guard<std::mutex> lock(mIndividualMutexes[thread]);
			mIndividualQueues[thread].emplace(task);
		}
	}

	void ThreadPool::YieldUntilEmpty() {
		YieldUntilCondition([this] {
			return IsQueueEmpty();
		});
	}

	void ThreadPool::YieldUntilCondition(const std::function<bool()>& predicate) {
		if (!predicate()) {
			ThreadProc(true, 0, &predicate);
		}
	}

	void ThreadPool::Startup(uint threads) {	
		bExit = false;

		mIndividualQueues.resize(threads);
		mIndividualMutexes = std::vector<std::mutex>(threads);

		for (uint i = 1; i < threads; ++i) {
			std::cout << "Initializing Thread " << i << std::endl;
			std::function<void()> threadProc = [this, i]() {
				ThreadProc(false, i, nullptr);
			};
			mThreads.emplace_back(std::thread(threadProc));
		}

		mTasksPending = 0;
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

			mIndividualMutexes.clear();
			mIndividualQueues.clear();
			mTaskQueue = queue_t();
			mOwnedTasks.clear();
		}
		bInitialized = false;
	}

	ITask* ThreadPool::Adopt(Task&& task) {
		auto ptr = task.Ptr();

		if (!ptr) {
			return nullptr;
		}

		{
			std::lock_guard<std::mutex> lock(mOwnedTasksMutex);
			mOwnedTasks.emplace(ptr, std::move(task));
		}

		return ptr;
	}
}