#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <set>
#include <memory>
#include <unordered_map>

#ifndef NDEBUG
// #define THREAD_POOL_DEBUG
#endif

#define THREAD_MAIN 0UL

typedef uint64_t TaskId;
typedef uint64_t ThreadMask;

namespace Morpheus {

	template <typename T>
	class Future;
	template <typename T>
	class UniqueFuture;
	template <typename T>
	class Promise;

	struct TaskNodeImpl;
	class NodeIn;
	class NodeOut;
	class IComputeQueue;
	class IBoundFunction;
	struct TaskNodeImpl;
	class TaskNode;

	class NodeIn {
	private:
		std::unique_lock<std::mutex> mLock;
		std::shared_ptr<TaskNodeImpl> mParent;

		void Connect(std::shared_ptr<TaskNodeImpl> join);

	public:
		NodeIn(std::shared_ptr<TaskNodeImpl> parent);

		NodeIn(const NodeIn&) = delete;
		NodeIn& operator=(const NodeIn&) = delete;

		NodeIn(NodeIn&&) = default;
		NodeIn& operator=(NodeIn&&) = default;

		NodeIn& SetStarted(bool value);
		NodeIn& SetScheduled(bool value);

		bool IsStarted() const;
		bool IsScheduled() const;
		bool IsLeaf() const;
		void Reset();
		bool Trigger();
		bool IsReady() const;

		friend class TaskNode;
		friend class NodeOut;
	};

	class NodeOut {
	private:
		std::unique_lock<std::mutex> mLock;
		std::shared_ptr<TaskNodeImpl> mParent;

		void Connect(std::shared_ptr<TaskNodeImpl> join);
	
	public:
		NodeOut(std::shared_ptr<TaskNodeImpl> parent);

		NodeOut(const NodeOut&) = delete;
		NodeOut& operator=(const NodeOut&) = delete;

		NodeOut(NodeOut&&) = default;
		NodeOut& operator=(NodeOut&&) = default;

		bool IsFinished() const;
		void SetFinished(bool value);
		void Fire(std::vector<TaskNode>& executable);
		void Reset();
		void Skip();

		friend class TaskNode;
	};

	class TaskNode {
	public:
		struct SearchResult {
			std::vector<TaskNode> mVisited;
			std::vector<TaskNode> mLeaves;
		};

	private:
		std::shared_ptr<TaskNodeImpl> mNode;

		enum SearchDirection {
			BACKWARD,
			FORWARD
		};

		template <SearchDirection dir>
		static SearchResult Search(const std::vector<TaskNode>& begin);

	public:
		TaskNode() = default;
		TaskNode(std::shared_ptr<TaskNodeImpl> node);

		std::shared_ptr<TaskNodeImpl> GetImpl();
		void SetFunction(std::unique_ptr<IBoundFunction>&& func);
		IBoundFunction* GetFunction() const;
		TaskNode& After(TaskNode other);
		TaskNode& Before(TaskNode other);
		void Execute(IComputeQueue* queue, int threadId);
		NodeIn In();
		NodeOut Out();
		const char* GetName() const;
		TaskNode& SetName(const char* name);
		TaskNode& SetPriority(int priority);
		TaskNode& SetThreadMask(ThreadMask mask);
		ThreadMask GetThreadMask() const;
		TaskNode& DisallowThread(uint threadId);
		TaskNode& OnlyThread(uint threadId);
		int GetPriority() const;
		TaskId GetId() const;
		void Reset();
		void ResetDescendants();
		void Skip();
		void SkipAncestors();

		operator bool() const;

		template <typename T>
		void After(const Future<T>& future);
		template <typename T>
		void After(const UniqueFuture<T>& future);
		template <typename T>
		void Before(const Promise<T>& promise);

		static TaskNode Create();
		static SearchResult BackwardSearch(const std::vector<TaskNode>& outputs);
		static SearchResult BackwardSearch(TaskNode output);
		static SearchResult ForwardSearch(const std::vector<TaskNode>& outputs);
		static SearchResult ForwardSearch(TaskNode output);

		struct Comparer {
			inline bool operator()(const TaskNode& lhs, const TaskNode& rhs) const {
				auto lhsPriority = lhs.GetPriority();
				auto rhsPriority = rhs.GetPriority();

				if (lhsPriority != rhsPriority) {
					return lhsPriority < rhsPriority;
				} else {
					return lhs.mNode < rhs.mNode;
				}
			}
		};

		friend class NodeOut;
	};

	template <>
	class Promise<void> {
	private:
		TaskNode mNode;
		
	public:
		Promise() : 
			mNode(TaskNode::Create()) {
		}

		TaskNode Node() {
			return mNode;
		}

		Promise(Promise<void>&& other) = default;
		Promise(const Promise& other) = default;
		Promise<void>& operator=(Promise<void>&& other) = default;
		Promise<void>& operator=(const Promise<void>& other) = default;

		friend class Future<void>;
		friend class TaskNode;
		friend class IComputeQueue;
	};

	template <typename T>
	class Promise {
	private:
		std::shared_ptr<T> mData;
		TaskNode mNode;
		
	public:
		Promise() : 
			mNode(TaskNode::Create()),
			mData(std::make_shared<T>()) {
		}

		Promise(T&& val) :
			mData(std::make_shared<T>(std::move(val))),
			mNode(TaskNode::Create()) {
		}

		Promise(const T& val) :
			mData(std::make_shared<T>(val)),
			mNode(TaskNode::Create()) {
		}

		TaskNode Node() {
			return mNode;
		}

		Promise(Promise&& other) = default;
		Promise(const Promise& other) = default;
		Promise<T>& operator=(Promise<T>&& other) = default;
		Promise<T>& operator=(const Promise<T>& other) = default;

		uint GetRefCount() const {
			return mData.use_count();
		}

		void Set(const T& value) {
			*mData = value;
			if (mNode.Out().IsFinished())
				mNode.Reset();
		}

		void Set(T&& value) { 
			*mData = std::move(value);
			if (mNode.Out().IsFinished())
				mNode.Reset();
		}

		Promise<T>& operator=(T& value) {
			Set(value);
			return *this;
		}

		Promise<T>& operator=(T&& value) {
			Set(std::move(value));
			return *this;
		}
		
		const T& Get() const {
			return *mData.get();
		}

		Future<T> GetFuture() const;
		UniqueFuture<T> GetUniqueFuture() const;

		friend class Future<T>;
		friend class UniqueFuture<T>;
		friend class TaskNode;
		friend class IComputeQueue;
	};

	template <>
	class Future<void> {
	private:
		TaskNode mNode;

	public:
		Future(const Promise<void>& promise) : 
			mNode(promise.mNode) {
		}

		TaskNode Node() {
			return mNode;
		}

		bool IsAvailable() {
			return mNode.Out().IsFinished();
		}

		bool IsFinished() {
			return mNode.Out().IsFinished();
		}

		inline void Evaluate() const;

		Future(Future<void>&& other) = default;
		Future(const Future<void>& other) = default;
		Future<void>& operator=(Future<void>&& other) = default;
		Future<void>& operator=(const Future<void>& other) = default;

		struct Comparer {
			bool operator()(const Future<void>& f1, const Future<void>& f2) {
				return f1.mNode.GetId() < f2.mNode.GetId();
			}
		};

		friend class TaskNode;
		friend class Task;
		friend class IComputeQueue;
	};

	template <typename T>
	class Future {
	private:
		std::shared_ptr<T> mData;
		TaskNode mNode;

	public:
		Future() {
		}

		Future(std::nullptr_t) {
		}

		Future(const Promise<T>& promise) : 
			mData(promise.mData),
			mNode(promise.mNode) {
		}

		Future(Future<T>&& other) = default;
		Future(const Future<T>& other) = default;
		Future<T>& operator=(Future<T>&& other) = default;
		Future<T>& operator=(const Future<T>& other) = default;

		uint RefCount() const {
			return mData.use_count();
		}

		TaskNode Node() {
			return mNode;
		}

		bool IsAvailable() {
			return mNode.Out().IsFinished();
		}

		const T& Get() const {
			return *mData.get();
		}

		operator bool() const {
			return mData != nullptr;
		}

		const T& Evaluate();

		struct Comparer {
			bool operator()(const Future<T>& f1, const Future<T>& f2) {
				return f1.mData < f2.mData;
			}
		};
		
		friend class TaskNode;
		friend class Task;
		friend class IComputeQueue;
	};

	template <typename T>
	class UniqueFuture {
	private:
		std::shared_ptr<T> mData;
		TaskNode mNode;

	public:
		UniqueFuture() {
		}

		UniqueFuture(const Promise<T>& promise) : 
			mData(promise.mData),
			mNode(promise.mNode) {
		}

		UniqueFuture(UniqueFuture<T>&& other) = default;
		UniqueFuture(const UniqueFuture<T>& other) = default;
		UniqueFuture<T>& operator=(UniqueFuture<T>&& other) = default;
		UniqueFuture<T>& operator=(const UniqueFuture<T>& other) = default;

		uint RefCount() const {
			return mData.use_count();
		}

		TaskNode Node() {
			return mNode;
		}

		bool IsAvailable() {
			return mNode.Out().IsFinished();
		}

		T& Get() {
			return *mData.get();
		}

		operator bool() const {
			return mData != nullptr;
		}

		T& Evaluate();

		struct Comparer {
			bool operator()(const UniqueFuture<T>& f1, const UniqueFuture<T>& f2) {
				return f1.mData < f2.mData;
			}
		};
		
		friend class TaskNode;
		friend class Task;
		friend class IComputeQueue;
	};

	typedef Promise<void> BarrierIn;
	typedef Future<void> BarrierOut;
	typedef Promise<void> Barrier;

	template <typename T>
	Future<T> Promise<T>::GetFuture() const {
		return Future<T>(*this);
	}

	template <typename T>
	UniqueFuture<T> Promise<T>::GetUniqueFuture() const {
		return UniqueFuture<T>(*this);
	}

	template <typename T>
	void TaskNode::After(const Future<T>& future) {
		After(future.mNode);
	}

	template <typename T>
	void TaskNode::After(const UniqueFuture<T>& future) {
		After(future.mNode);
	}

	template <typename T>
	void TaskNode::Before(const Promise<T>& promise) {
		Before(promise.mNode);
	}
	
	struct TaskParams {
		IBoundFunction* mCurrent;
		IComputeQueue* mQueue;
		uint32_t mThread;
	};

	class IBoundFunction {
	private:
		std::shared_ptr<TaskNodeImpl> mNode;

	public:
		virtual void Execute(IComputeQueue* queue, int threadId) = 0;
		TaskNode Node();
	};

	template <typename T, typename... Args>
	class BoundFunction : public IBoundFunction {
	private:
		T mLambda;
		std::tuple<Args...> mArgs;

	public:
		BoundFunction(const T& lambda, std::tuple<Args...>&& args) : 
			mLambda(lambda), mArgs(std::move(args)) {
		}

		void Execute(IComputeQueue* queue, int threadId) override {
			TaskParams params;
			params.mQueue = queue;
			params.mThread = threadId;
			params.mCurrent = this;

			std::apply([this, &params, &lambda = mLambda](const Args&... args) {
				lambda(params, args...);
			}, mArgs);
		}
	};

	template <typename First, typename... Rest>
	struct FunctionArgsBinder {
	};

	template <typename T>
	struct FunctionArgsBinder<Future<T>> {
		static void Bind(TaskNode& node, Future<T>& future) {
			if (future)
				node.After(future);
		}
	};

	template <typename T>
	struct FunctionArgsBinder<UniqueFuture<T>> {
		static void Bind(TaskNode& node, UniqueFuture<T>& future) {
			if (future)
				node.After(future);
		}
	};

	template <typename T>
	struct FunctionArgsBinder<Promise<T>> {
		static void Bind(TaskNode& node, Promise<T>& promise) {
			node.Before(promise);
		}
	};

	template <typename T, typename... Rest>
	struct FunctionArgsBinder<Future<T>, Rest...> {
		static void Bind(TaskNode& node, Future<T>& future, Rest... rest) {
			FunctionArgsBinder<Future<T>>::Bind(node, future);
			FunctionArgsBinder<Rest...>::Bind(node, rest...);
		}
	};

	template <typename T, typename... Rest>
	struct FunctionArgsBinder<UniqueFuture<T>, Rest...> {
		static void Bind(TaskNode& node, UniqueFuture<T>& future, Rest... rest) {
			FunctionArgsBinder<UniqueFuture<T>>::Bind(node, future);
			FunctionArgsBinder<Rest...>::Bind(node, rest...);
		}
	};

	template <typename T, typename... Rest>
	struct FunctionArgsBinder<Promise<T>, Rest...> {
		static void Bind(TaskNode& node, Promise<T>& promise, Rest... rest) {
			FunctionArgsBinder<Promise<T>>::Bind(node, promise);
			FunctionArgsBinder<Rest...>::Bind(node, rest...);
		}
	};

	template <typename... Args>
	class ILambdaWrap {
	public:
		virtual void operator()(const TaskParams& e, Args... args) = 0;
		virtual std::unique_ptr<IBoundFunction> Bind(Args... args) = 0;
	};

	template <typename T, typename... Args>
	class LambdaWrap : public ILambdaWrap<Args...> {
		T mLambda;

	public:
		LambdaWrap(T&& lambda) : mLambda(std::move(lambda)) {
		}

		void operator()(const TaskParams& e, Args... args) override {
			mLambda(e, args...);
		}

		std::unique_ptr<IBoundFunction> Bind(Args... args) override {
			return std::make_unique<BoundFunction<T, Args...>>(mLambda,
				std::make_tuple<Args...>(std::move(args)...));
		}
	};

	template <typename... Args>
	class FunctionPrototype {
	private:
		std::unique_ptr<ILambdaWrap<Args...>> mLambda;

	public:
		template <typename T>
		FunctionPrototype(T&& lambda) :
			mLambda(std::make_unique<LambdaWrap<T, Args...>>(std::move(lambda))) {
		}

		TaskNode operator()(Args... args) {
			auto result = TaskNode::Create();

			// Bind the arguments to the task node
			FunctionArgsBinder<Args...>::Bind(result, args...);

			// Create the bound function
			result.SetFunction(mLambda->Bind(std::move(args)...));

			return result;
		}
	};

	template <>
	class FunctionPrototype<> {
	private:
		std::unique_ptr<ILambdaWrap<>> mLambda;

	public:
		template <typename T>
		FunctionPrototype(T&& lambda) :
			mLambda(std::make_unique<LambdaWrap<T>>(std::move(lambda))) {
		}

		TaskNode operator()() {
			auto result = TaskNode::Create();

			// Create the bound function
			result.SetFunction(mLambda->Bind());

			return result;
		}
	};

	class Task {
	private:
		TaskNode mTaskStart;
		TaskNode mTaskEnd;
		std::vector<TaskNode> mInternalNodes;
		std::vector<std::shared_ptr<Task>> mInternalTasks;

	protected:
		void Add(TaskNode node);
		void Add(std::shared_ptr<Task> task);

	public:
		Task();
		Task(TaskNode node);

		virtual ~Task() = default;

		const std::vector<TaskNode>& Inputs() const;

		TaskNode InNode() const;
		TaskNode OutNode() const;

		NodeIn In();
		NodeOut Out();

		void Reset();
		void Skip();

		void After(TaskNode node);
		void Before(TaskNode node);

		void After(const Task& task);
		void Before(const Task& task);

		void operator()();

		friend class IComputeQueue;
	};

	class CustomTask : public Task {
	public:
		void Add(TaskNode node);
		void Add(std::shared_ptr<Task> task);
	};

	class IComputeQueue {
	protected:
		virtual void SubmitImpl(TaskNode node) = 0;

	public:
		void Submit(TaskNode node);
		void Submit(const Task& task);

		template <typename T>
		void Submit(const Future<T>& future) {
			SubmitImpl(future.mNode);
		}

		template <typename T>
		void Submit(const UniqueFuture<T>& future) {
			SubmitImpl(future.mNode);
		}

		inline void Submit(const Future<void>& future) {
			SubmitImpl(future.mNode);
		}

		virtual void YieldUntilCondition(const std::function<bool()>& predicate) = 0;
		virtual void YieldUntilEmpty() = 0;

		void YieldFor(const std::chrono::high_resolution_clock::duration& duration);
		void YieldUntil(const std::chrono::high_resolution_clock::time_point& time);
		void YieldUntilFinished(TaskNode node);

		template <typename T>
		void YieldUntilFinished(const Future<T>& future) {
			YieldUntilFinished(future.mNode);
		}

		inline void YieldUntilFinished(const Future<void>& future) {
			YieldUntilFinished(future.mNode);
		}

		template <typename T>
		void YieldUntilFinished(const UniqueFuture<T>& future) {
			YieldUntilFinished(future.mNode);
		}

		template <typename T>
		const T& Evaluate(const Future<T>& future) {
			Submit(future);
			YieldUntilFinished(future);
			return future.Get();
		}

		template <typename T>
		T& Evaluate(const UniqueFuture<T>& future) {
			Submit(future);
			YieldUntilFinished(future);
			return future.Get();
		}

		inline void Evaluate(const Future<void>& future) {
			Submit(future);
			YieldUntilFinished(future);
		}

		inline void Evaluate(TaskNode node) {
			Submit(node);
			YieldUntilFinished(node);
		}

		template <typename T>
		T& Evaluate(UniqueFuture<T>& future) {
			Submit(future);
			YieldUntilFinished(future);
			return future.Get();
		}

		void YieldUntilFinished(const Task& task);
	};

	class ImmediateComputeQueue : public IComputeQueue {
	private:
		std::queue<TaskNode> mImmediateQueue;

	protected:
		void RunOneJob();
		void SubmitImpl(TaskNode node) override;

	public:
		~ImmediateComputeQueue() = default;
		
		void YieldUntilCondition(const std::function<bool()>& predicate) override;
		void YieldUntilEmpty() override;

		size_t RemainingJobCount() const;
	};

	class ThreadPool : public IComputeQueue {
	public:
		using queue_t = std::set<TaskNode, 
			typename TaskNode::Comparer>;
		using gaurd_t = std::lock_guard<std::mutex>;

	private:
		bool bInitialized;
		std::vector<std::thread> mThreads;
		std::atomic<bool> bExit;
		std::atomic<uint> mTasksPending;

		std::mutex mCollectiveQueueMutex;
		queue_t mCollectiveQueue;

		queue_t mTaskQueue;

		void ThreadProc(bool bIsMainThread, uint threadNumber, const std::function<bool()>* finishPredicate);

	protected:
		void SubmitImpl(TaskNode node) override;

	public:
		bool IsQueueEmpty();
		uint ThreadCount() const;
		ThreadPool();
		~ThreadPool();

		void YieldUntilCondition(const std::function<bool()>& predicate) override;
		void YieldUntilEmpty() override;
	
		void Startup(uint threads = std::thread::hardware_concurrency());
		void Shutdown();

		friend class TaskQueueInterface;
		friend class TaskBarrier;
		friend class TaskNodeDependencies;
	};

	template <typename T>
	const T& Future<T>::Evaluate() {
		return ImmediateComputeQueue().Evaluate(*this);
	}

	template <typename T>
	T& UniqueFuture<T>::Evaluate() {
		return ImmediateComputeQueue().Evaluate(*this);
	}

	void Future<void>::Evaluate() const {
		ImmediateComputeQueue().Evaluate(*this);
	}
}