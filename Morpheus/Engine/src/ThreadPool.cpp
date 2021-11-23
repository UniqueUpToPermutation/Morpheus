#include <Engine/ThreadPool.hpp>

#include <iostream>

std::atomic<uint64_t> gCurrentTaskId;

#ifdef THREAD_POOL_DEBUG
#include <iostream>
std::mutex gPoolOutput;
#endif

#include "Fence.h"

namespace DG = Diligent;

namespace Morpheus {
	struct TaskNodeImpl {
		std::vector<std::shared_ptr<TaskNodeImpl>> mIn;
		std::vector<std::shared_ptr<TaskNodeImpl>> mOut;
		std::mutex mInMutex;
		std::mutex mOutMutex;
		uint32_t mInLeft = 0;
		bool bStarted = false;
		bool bScheduled = false;
		bool bIsFinished = false;
		int mPriority = 0;
		ThreadMask mThreadMask = ~(ThreadMask)0;
		TaskId mId;
		const char* mName = nullptr;
		DG::IFence* mFence = nullptr;
		DG::Uint64 mFenceCompletedValue;

		std::unique_ptr<IBoundFunction> mBoundFunction;
	};

	TaskNode::SearchResult TaskNode::BackwardSearch(TaskNode output) {
		return BackwardSearch(std::vector<TaskNode>{output});
	}

	template <TaskNode::SearchDirection dir>
	TaskNode::SearchResult TaskNode::Search(
		const std::vector<TaskNode>& begin) {
		
		std::queue<TaskNode> queue;
		std::queue<TaskNode> newItems;
		std::set<TaskId> visited;
		TaskNode::SearchResult result;

		// Enqueue helper
		auto enqueue = [&queue, &visited, &result](TaskNode node) {
			bool bLeaf = false;
			bool bEmplace = false;

			if constexpr (dir == SearchDirection::BACKWARD) {
				auto inLock = node.In();
				bLeaf = inLock.IsLeaf();
				bEmplace = !inLock.IsScheduled();
			} else if constexpr (dir == SearchDirection::FORWARD) {
				auto outLock = node.Out();
				bEmplace = outLock.IsFinished();
			}

			if (bEmplace) {
				auto it = visited.find(node.GetId());

				if (it == visited.end()) {
					queue.emplace(node);
					visited.emplace_hint(it, node.GetId());
					result.mVisited.emplace_back(node);

					if (bLeaf) {
						result.mLeaves.emplace_back(node);
					}
				}
			}
		};

		// Enqueue starting nodes
		for (auto node : begin) {
			enqueue(node);
		}

		while (!queue.empty()) {
			TaskNode top = queue.front();
			queue.pop();

			// Prepare to enqueue children
			if constexpr (dir == SearchDirection::BACKWARD) {
				auto inLock = top.In();
				for (auto& item : top.mNode->mIn) {
					newItems.emplace(item);
				}
			} else if constexpr (dir == SearchDirection::FORWARD) {
				auto outLock = top.Out();
				for (auto& item : top.mNode->mOut) {
					newItems.emplace(item);
				}
			}

			// Actually enqueue children
			while (!newItems.empty()) {
				auto node = newItems.front();
				newItems.pop();
				enqueue(node);
			}
		}

		return result;
	}

	TaskNode::SearchResult TaskNode::ForwardSearch(const std::vector<TaskNode>& outputs) {
		return Search<SearchDirection::FORWARD>(outputs);
	}

	TaskNode::SearchResult TaskNode::ForwardSearch(TaskNode output) {
		return ForwardSearch(std::vector<TaskNode>{output});
	}

	TaskNode::SearchResult TaskNode::BackwardSearch(
		const std::vector<TaskNode>& outputs) {
		return Search<SearchDirection::BACKWARD>(outputs);
	}

	NodeIn::NodeIn(std::shared_ptr<TaskNodeImpl> parent) :
		 mParent(parent), mLock(parent->mInMutex) {
	}

	bool NodeIn::IsStarted() const {
		return mParent->bStarted;
	}

	void NodeIn::Connect(std::shared_ptr<TaskNodeImpl> join) {
		mParent->mIn.emplace_back(join);
		mParent->mInLeft++;
	}

	NodeIn& NodeIn::SetStarted(bool value) {
		mParent->bStarted = value;
		return *this;
	}

	NodeIn& NodeIn::SetScheduled(bool value) {
		mParent->bScheduled = value;
		return *this;
	}

	void NodeIn::Reset() {
		mParent->bStarted = false;
		mParent->bScheduled = false;
	}

	bool NodeIn::Trigger() {
		mParent->mInLeft--;

		if (IsReady())
			return true;

		return false;
	}

	bool NodeIn::IsLeaf() const {
		return mParent->mInLeft == 0 && !mParent->bStarted;
	}

	bool NodeIn::IsReady() const {
		return IsLeaf() && IsScheduled();
	}
	
	bool NodeIn::IsScheduled() const {
		return mParent->bScheduled;
	}

	NodeOut::NodeOut(std::shared_ptr<TaskNodeImpl> parent) : 
		mParent(parent), mLock(parent->mOutMutex) {
	}

	bool NodeOut::IsFinished() const {
		return mParent->bIsFinished;
	}

	void NodeOut::Connect(std::shared_ptr<TaskNodeImpl> join) {
		mParent->mOut.emplace_back(join);
	}

	void NodeOut::SetFinished(bool value) {
		mParent->bIsFinished = value;
	}

	void NodeOut::Fire(std::vector<TaskNode>& executable) {
		mParent->bIsFinished = true;
		for (auto& target : mParent->mOut) {
			if (NodeIn(target).Trigger()) {
				executable.emplace_back(target);
			}
		}
	}

	void NodeOut::Skip() {
		mParent->bIsFinished = true;
		for (auto& target : mParent->mOut) {
			auto in = NodeIn(target);
			target->mInLeft--;
		}
	}

	NodeIn TaskNode::In() {
		return NodeIn(mNode);
	}

	NodeOut TaskNode::Out() {
		return NodeOut(mNode);
	}

	void TaskNode::SetFunction(std::unique_ptr<IBoundFunction>&& func) {
		mNode->mBoundFunction = std::move(func);
	}

	IBoundFunction* TaskNode::GetFunction() const {
		return mNode->mBoundFunction.get();
	}

	std::shared_ptr<TaskNodeImpl> TaskNode::GetImpl() {
		return mNode;
	}

	TaskNode& TaskNode::After(TaskNode other) {
		other.Out().Connect(mNode);
		In().Connect(other.mNode);
		return *this;
	}

	int TaskNode::GetPriority() const {
		return mNode->mPriority;
	}
	
	TaskId TaskNode::GetId() const {
		return mNode->mId;
	}

	void TaskNode::Reset() {
		In().Reset();
		Out().Reset();
	}

	void TaskNode::ResetDescendants() {
		SearchResult result = ForwardSearch(*this);

		for (auto& n : result.mVisited) {
			n.Reset();
		}
	}

	void TaskNode::Skip() {
		Out().Skip();
	}

	void TaskNode::SkipAncestors() {
		SearchResult result = BackwardSearch(*this);

		for (auto& n : result.mVisited) {
			n.Skip();
		}
	}

	TaskNode::operator bool() const {
		return mNode != nullptr;
	}

	TaskNode& TaskNode::SetPriority(int priority) {
		mNode->mPriority = priority;
		return *this;
	}

	TaskNode& TaskNode::SetThreadMask(ThreadMask mask) {
		mNode->mThreadMask = mask;
		return *this;
	}

	ThreadMask TaskNode::GetThreadMask() const {
		return mNode->mThreadMask;
	}

	TaskNode& TaskNode::DisallowThread(uint threadId) {
		mNode->mThreadMask &= ~((ThreadMask)1 << threadId);
		return *this;
	}

	TaskNode& TaskNode::OnlyThread(uint threadId) {
		mNode->mThreadMask = (ThreadMask)1 << threadId;
		return *this;
	}

	TaskNode& TaskNode::Before(TaskNode other) {
		Out().Connect(other.mNode);
		other.In().Connect(mNode);
		return *this;
	}

	void NodeOut::Reset() {
		if (mParent->bIsFinished) {
			mParent->bIsFinished = false;

			for (auto& child : mParent->mOut) {
				auto inLock = TaskNode(child).In();
				child->mInLeft++;
			}
		}
	}

	void TaskNode::Execute(IComputeQueue* queue, int threadId) {
		if (mNode->mBoundFunction) {
			mNode->mBoundFunction->Execute(queue, threadId);
		}
	}

	const char* TaskNode::GetName() const {
		return mNode->mName;
	}

	TaskNode& TaskNode::SetName(const char* name) {
		mNode->mName = name;
		return *this;
	}

	TaskNode::TaskNode(std::shared_ptr<TaskNodeImpl> node) :
		mNode(node) {
	}

	TaskNode IBoundFunction::Node() {
		return TaskNode(mNode);
	}

	TaskNode TaskNode::Create() {
		auto result = std::make_shared<TaskNodeImpl>();
		result->mId = gCurrentTaskId.fetch_add(1);
		return TaskNode(result);
	}

	NodeIn Task::In() {
		return mTaskStart.In();
	}

	NodeOut Task::Out() {
		return mTaskEnd.Out();
	}

	TaskNode Task::InNode() const {
		return mTaskStart;
	}

	TaskNode Task::OutNode() const {
		return mTaskEnd;
	}

	Task::Task(TaskNode node) :
		Task() {
		Add(node);
	}

	Task::Task() : 
		mTaskStart(TaskNode::Create()),
		mTaskEnd(TaskNode::Create()) {
		mTaskStart.SetName("[TASK START]");
		mTaskEnd.SetName("[TASK END]");
		mTaskEnd.After(mTaskStart);
	}

	void Task::After(TaskNode node) {
		mTaskStart.After(node);
	}

	void Task::Reset() {
		mTaskStart.Reset();
		for (auto internal : mInternalNodes) {
			internal.Reset();
		}
		for (auto internal : mInternalTasks) {
			internal->Reset();
		}
		mTaskEnd.Reset();
	}

	void Task::Skip() {
		mTaskEnd.Skip();
	}

	void Task::Before(TaskNode node) {
		mTaskEnd.Before(node);
	}

	void Task::After(const Task& task) {
		mTaskStart.After(task.mTaskEnd);
	}

	void Task::Before(const Task& task) {
		mTaskEnd.Before(task.mTaskStart);
	}

	void Task::Add(TaskNode node) {
		mTaskEnd.After(node);
		mTaskStart.Before(node);
		mInternalNodes.emplace_back(node);
	}

	void Task::Add(std::shared_ptr<Task> task) {
		mTaskEnd.After(task->mTaskEnd);
		mTaskStart.Before(task->mTaskStart);
		mInternalTasks.emplace_back(task);
	}

	void Task::operator()() {
		ImmediateComputeQueue queue;
		queue.Submit(OutNode());
		queue.YieldUntilEmpty();
	}

	void IComputeQueue::Submit(TaskNode node) {
		SubmitImpl(node);
	}

	void IComputeQueue::Submit(const Task& task) {
		SubmitImpl(task.OutNode());
	}

	void IComputeQueue::YieldFor(const std::chrono::high_resolution_clock::duration& duration) {
		auto start = std::chrono::high_resolution_clock::now();
		
		YieldUntilCondition([start, duration]() {
			auto now = std::chrono::high_resolution_clock::now();
			return (now - start) > duration;
		});
	}

	void IComputeQueue::YieldUntil(const std::chrono::high_resolution_clock::time_point& time) {
		YieldUntilCondition([time]() {
			auto now = std::chrono::high_resolution_clock::now();
			return now > time;
		});
	}

	void IComputeQueue::YieldUntilFinished(TaskNode node) {
		YieldUntilCondition([&node](){
			return node.Out().IsFinished();
		});
	}

	void IComputeQueue::YieldUntilFinished(const Task& task) {
		YieldUntilFinished(task.OutNode());
	}

	void ImmediateComputeQueue::SubmitImpl(TaskNode node) {
		if (!node.In().IsStarted()) {
			auto toTrigger = TaskNode::BackwardSearch(node);

			for (auto& node : toTrigger.mVisited) {
				node.In().SetScheduled(true);
			}

			for (auto& node : toTrigger.mLeaves) {
				mImmediateQueue.emplace(node);
			}
		}
	}

	std::vector<TaskNode> ImmediateComputeQueue::CheckGPUJobs() {
		std::vector<TaskNode> result;

		for (auto it = mWaitOnGpu.begin(); it != mWaitOnGpu.end();) {
			auto node = *it;
			auto impl = node.GetImpl();
			if (impl->mFence->GetCompletedValue() == impl->mFenceCompletedValue) {
				result.emplace_back(node);
				auto last = it++;
				mWaitOnGpu.erase(last);
			} 
		}

		return result;
	}

	void ImmediateComputeQueue::RunOneJob() {
		if (mWaitOnGpu.size() > 0) {
			auto gpuJobsToTrigger = CheckGPUJobs();
			for (auto& job : gpuJobsToTrigger) {
				mImmediateQueue.emplace(job);
			}
		}

		if (mImmediateQueue.size() == 0)
			return;

		TaskNode node = std::move(mImmediateQueue.front());
		mImmediateQueue.pop();

		std::vector<TaskNode> next;

		// Signal that we've started this task!
		{
#ifdef THREAD_POOL_DEBUG
			{
				std::lock_guard<std::mutex> lock2(gPoolOutput);

				if (node.GetName()) {
					std::cout << "Arrived at node: " 
						<< node.GetName() << std::endl;
				} else {
					std::cout << "Arrived at node: " 
						<< "[UNNAMED]" << std::endl;
				}
			}
#endif

			auto inLock = node.In();

			// Task has already been started. Don't start twice!
			if (inLock.IsStarted())
				return;

			inLock.SetStarted(true);
		}

		bool bExecute = true;

		// Node has a GPU fence!
		// Make sure that fence has been completed before running
		auto impl = node.GetImpl();
		if (impl->mFence && impl->mFence->GetCompletedValue() != impl->mFenceCompletedValue) {
			bExecute = false;
			node.In().SetStarted(false);
			mWaitOnGpu.emplace(node);
		}

		if (bExecute) {
			node.Execute(this, THREAD_MAIN);
			node.Out().Fire(next);

	#ifdef THREAD_POOL_DEBUG
			{
				std::lock_guard<std::mutex> lock2(gPoolOutput);

				if (node.GetName()) {
					std::cout << "Leaving at node: " 
						<< node.GetName() << std::endl;
				} else {
					std::cout << "Leaving at node: " 
						<< "[UNNAMED]" << std::endl;
				}
			}
	#endif

			for (auto& n : next)
				mImmediateQueue.push(n);
		}
	}

	void ImmediateComputeQueue::YieldUntilCondition(const std::function<bool()>& predicate) {
		while (!predicate()) {
			RunOneJob();
		}
	}

	void ImmediateComputeQueue::YieldUntilEmpty() {
		YieldUntilCondition([this]{
			return RemainingJobCount() == 0;
		});
	}

	size_t ImmediateComputeQueue::RemainingJobCount() const {
		return mImmediateQueue.size();
	}

	void CustomTask::Add(TaskNode node) {
		Task::Add(node);
	}

	void CustomTask::Add(std::shared_ptr<Task> task) {
		Task::Add(task);
	}

	bool ThreadPool::IsQueueEmpty() {
		return mTasksPending == 0;
	}

	uint ThreadPool::ThreadCount() const {
		return mThreads.size() + 1;
	}

	ThreadPool::ThreadPool() : 
		bInitialized(false),
		bExit(false) {
		mTasksPending = 0;
	}

	ThreadPool::~ThreadPool() {
		Shutdown();
	}

	void ThreadPool::SubmitImpl(TaskNode node) {
		if (!node.In().IsStarted()) {
			auto toTrigger = TaskNode::BackwardSearch(node);

			for (auto& node : toTrigger.mVisited) {
				node.In().SetScheduled(true);
			}

			{
				std::unique_lock<std::mutex> lock(mCollectiveQueueMutex);
				for (auto& node : toTrigger.mLeaves) {
					mCollectiveQueue.emplace(node);
					++mTasksPending;
				}
			}
		}
	}

	void ThreadPool::YieldUntilCondition(const std::function<bool()>& predicate) {
		if (!predicate()) {
			ThreadProc(true, 0, &predicate);
		}
	}

	void ThreadPool::YieldUntilEmpty() {
		YieldUntilCondition([this] {
			return IsQueueEmpty();
		});
	}
	
	void ThreadPool::Startup(uint threads) {
		bExit = false;

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

			mTaskQueue = queue_t();
		}
		bInitialized = false;
	}

	void ThreadPool::ThreadProc(bool bIsMainThread, 
		uint threadNumber, 
		const std::function<bool()>* finishPredicate) {
		
		ThreadMask mask = (ThreadMask)1 << threadNumber;

		while (!bExit) {
			// We have reached our finish predicate
			if (finishPredicate && (*finishPredicate)()) {
				break;
			}

			TaskNode task;

			// Check GPU fence on main thread.
			if (threadNumber == THREAD_MAIN) {
				std::lock_guard<std::mutex> lockWaitGpu(mWaitOnGpuMutex);
				std::unique_lock<std::mutex> lockQueue;

				for (auto it = mWaitOnGpu.begin(); it != mWaitOnGpu.end();) {
					auto node = *it;
					auto impl = node.GetImpl();

					if (impl->mFence->GetCompletedValue() == impl->mFenceCompletedValue) {
						auto last = it++;
						mWaitOnGpu.erase(last);
						lockQueue = std::unique_lock<std::mutex>(mCollectiveQueueMutex);
						mCollectiveQueue.emplace(node);
					}
				}
			}

			{
				// Select a task assigned to the collective queue
				std::lock_guard<std::mutex> lock(mCollectiveQueueMutex);

				for (auto it = mCollectiveQueue.begin(); 
					it != mCollectiveQueue.end(); ++it) {

					if (it->GetThreadMask() | mask) {
						task = *it;
						mCollectiveQueue.erase(it);
						break;
					}
				}
			}

			if (task) {

				std::vector<TaskNode> next;

				// Signal that we've started this task!
				{
#ifdef THREAD_POOL_DEBUG
					{
						std::lock_guard<std::mutex> lock2(gPoolOutput);

						if (task.GetName()) {
							std::cout << "Arrived at node: " 
								<< task.GetName() << std::endl;
						} else {
							std::cout << "Arrived at node: " 
								<< "[UNNAMED]" << std::endl;
						}
					}
#endif

					auto inLock = task.In();

					// Task has already been started. Don't start twice!
					if (inLock.IsStarted())
						task = TaskNode();

					inLock.SetStarted(true);
				}

				// Node has a GPU fence!
				// Make sure that fence has been completed before running
				auto impl = task.GetImpl();
				if (impl->mFence && impl->mFence->GetCompletedValue() != impl->mFenceCompletedValue) {
					task.In().SetStarted(false);
					{
						std::lock_guard<std::mutex> lock(mWaitOnGpuMutex);
						mWaitOnGpu.emplace(task);
					}
					task = TaskNode();
				}

				if (task) {
					task.Execute(this, threadNumber);
					task.Out().Fire(next);

#ifdef THREAD_POOL_DEBUG
					{
						std::lock_guard<std::mutex> lock2(gPoolOutput);

						if (task.GetName()) {
							std::cout << "Leaving at node: " 
								<< task.GetName() << std::endl;
						} else {
							std::cout << "Leaving at node: " 
								<< "[UNNAMED]" << std::endl;
						}
					}
#endif

					{
						std::unique_lock<std::mutex> lock(mCollectiveQueueMutex);
						for (auto& n : next) {
							mCollectiveQueue.emplace(n);
							++mTasksPending;
						}
					}
				}

				--mTasksPending;
				
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

	Promise<void>::Promise(Diligent::IFence* fence, uint64_t fenceCompletedValue) : mNode(TaskNode::Create()) {
		auto impl = mNode.GetImpl();
		impl->mFence = fence;
		impl->mFenceCompletedValue = fenceCompletedValue;
	}
}