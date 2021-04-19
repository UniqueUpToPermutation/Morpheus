#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <future>

#define ASSIGN_THREAD_ANY -1
#define ASSIGN_THREAD_MAIN 0

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

	// declare a non-polymorphic container for any function object that takes zero args and returns an int
	// in addition, the contained function need not be copyable
	class TaskFunc
	{
		// define the concept of being callable while mutable
		struct concept
		{
			concept() = default;
			concept(concept&&) = default;
			concept& operator=(concept&&) = default;
			concept(const concept&) = delete;
			concept& operator=(const concept&) = default;
			virtual ~concept() = default;

			virtual void call(const TaskParams& e) = 0;
		};

		// model the concept for any given function object
		template<class F>
		struct model : concept
		{
			model(F&& f)
			: _f(std::move(f))
			{}

			void call(const TaskParams& e) override
			{
				_f(e);
			}

			F _f;
		};

	public:
		// provide a public interface
		void operator()(const TaskParams& e)  // note: not const
		{
			return _ptr->call(e);
		}

		// provide a constructor taking any appropriate object
		template<class FI>
		TaskFunc(FI&& f)
		: _ptr(std::make_unique<model<FI>>(std::move(f)))
		{}

		TaskFunc(nullptr_t ptr) {
		}

		TaskFunc() {
		}

		inline bool valid() const {
			return _ptr != nullptr;
		}

	private:
		std::unique_ptr<concept> _ptr;
	};

	class ITaskQueue;

	struct TaskParams {
		ITaskQueue* mQueue;
		uint mThreadId;
	};

	struct Task {
	public:
		TaskSyncPoint* mSyncPoint;
		int mAssignedThread;
		TaskFunc mFunc;
		TaskType mType;

		Task(TaskFunc&& func, 
			TaskType type = TaskType::UNSPECIFIED,
			TaskSyncPoint* syncPoint = nullptr, 
			int assignedThread = ASSIGN_THREAD_ANY) :
			mFunc(std::move(func)),
			mType(type),
			mSyncPoint(syncPoint),
			mAssignedThread(assignedThread) {
		}

		Task() :
			mFunc(),
			mType(TaskType::UNSPECIFIED),
			mSyncPoint(nullptr),
			mAssignedThread(ASSIGN_THREAD_ANY) {
		}

		inline bool IsValid() const {
			return mFunc.valid();
		}

		void operator()();
		void operator()(const TaskParams& e);
	};


	struct TaskSyncPoint {
	private:
		std::atomic<uint> mAwaiting;
		Task mCallback;

	public:
		TaskSyncPoint() :
			mCallback(nullptr) {
			mAwaiting = 0;
		}

		inline void SetCallback(Task&& callback) {
			mCallback = std::move(callback);
		}

		inline bool IsFinished() const {
			return mAwaiting == 0;
		}

		inline uint StartNewTask() {
			return mAwaiting.fetch_add(1) + 1;
		}

		inline uint EndTask(const TaskParams& params);
	};

	class ITaskQueue {
	public:
		virtual void Submit(Task&& taskDesc) = 0;
		
		inline void Submit(TaskFunc&& task,
			TaskType taskType = TaskType::UNSPECIFIED,
			TaskSyncPoint* syncPoint = nullptr, 
			int assignedThread = ASSIGN_THREAD_ANY) {
			Submit(Task(std::move(task), taskType, syncPoint, assignedThread));
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
		
		inline void YieldUntil(const TaskSyncPoint& syncPoint) {
			YieldUntilCondition([&]() {
				return syncPoint.IsFinished();
			});
		}

		inline void YieldUntil(const TaskSyncPoint* syncPoint) {
			YieldUntilCondition([=]() {
				return syncPoint->IsFinished();
			});
		}

		template <typename T>
		inline void YieldUntil(const std::future<T>& future) {
			YieldUntilCondition([&]() {
				return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
			});
		}
	};

	class ImmediateJobQueue : public ITaskQueue {
	public:
		void Submit(Task&& taskDesc) override;
		void YieldUntilCondition(const std::function<bool()>& predicate) override;
	};

	struct TaskCompare
	{
		inline bool operator()(const Task& lhs, const Task& rhs)
		{
			auto lhs_val = GetTaskPriority(lhs.mType);
			auto rhs_val = GetTaskPriority(rhs.mType);

			return lhs_val > rhs_val;
		}
	};

	class ThreadPool : public ITaskQueue {
	public:
		using queue_t = std::priority_queue<Task, std::vector<Task>, TaskCompare>;

	private:
		bool bInitialized;
		std::vector<std::thread> mThreads;
	
		std::mutex mMutex;

		std::atomic<bool> bExit;
		std::vector<queue_t> mIndividualImmediateQueues;
		queue_t mSharedImmediateTasks;

		void ThreadProc(bool bIsMainThread, uint threadNumber, const std::function<bool()>* finishPredicate);

	public:
		inline uint ThreadCount() const {
			return mThreads.size() + 1;
		}

		inline ThreadPool() : 
			bInitialized(false),
			bExit(false) {
		}

		inline ~ThreadPool() {
			Shutdown();
		}

		void Submit(Task&& taskDesc) override;
		
		void YieldUntilCondition(const std::function<bool()>& predicate) override;
		void YieldUntilFinished();

		void Startup(uint threads = std::thread::hardware_concurrency());
		void Shutdown();

		friend class TaskQueueInterface;
		friend class TaskBarrier;
		friend class TaskNodeDependencies;
	};

	uint TaskSyncPoint::EndTask(const TaskParams& params) {
		auto result = mAwaiting.fetch_sub(1) - 1;

		if (result == 0 && mCallback.IsValid()) {
			params.mQueue->Submit(std::move(mCallback));
		}

		return result;
	}
}