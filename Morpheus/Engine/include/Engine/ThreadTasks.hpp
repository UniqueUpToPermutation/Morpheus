#pragma once

#include <Engine/ThreadPool.hpp>

namespace Morpheus {
	struct ReadFileToMemoryResult {
	private:
		unsigned char* mData;
		size_t mSize;

	public:
		inline unsigned char* GetData() {
			return mData;
		}

		inline size_t GetSize() const {
			return mSize;
		}

		inline ReadFileToMemoryResult(unsigned char* data, size_t size) :
			mData(data),
			mSize(size) {
		}

		inline ~ReadFileToMemoryResult() {
			delete[] mData;
		}

		ReadFileToMemoryResult(const ReadFileToMemoryResult& other) = delete;
		
		inline ReadFileToMemoryResult(ReadFileToMemoryResult&& other) {
			std::swap(mData, other.mData);
			std::swap(mSize, other.mSize);
		}
	};

	TaskId ReadFileToMemoryJobDeferred(const std::string& filename, 
		TaskQueueInterface* queue, 
		PipeId* pipeOut,
		TaskBarrier* barrier = nullptr,
		bool binary = true);
}