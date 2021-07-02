#pragma once

#include <set>
#include <functional>

namespace Morpheus {
	class PoolBase {
	private:
		std::set<int> mFreeInHeap;
		int mHeapSize;
		int mTotalSize;

	public:
		inline PoolBase(uint size) :
			mTotalSize(size),
			mHeapSize(0) {
		}

		inline uint HeapSize() const {
			return mHeapSize;
		}

		inline uint TotalSize() const {
			return mTotalSize;
		}

		inline uint FreeSize() const {
			return mTotalSize - mHeapSize + mFreeInHeap.size();
		}

		inline void Resize(uint newSize) {
			if (newSize < mHeapSize)
				throw std::runtime_error("New size is too small to fit pool heap!");

			mTotalSize = newSize;
		}

		inline int Alloc() {
			if (FreeSize() == 0)
				throw std::runtime_error("Pool is full!");

			if (mFreeInHeap.size()) {
				auto smallestIt = mFreeInHeap.begin();
				auto value = *smallestIt;
				mFreeInHeap.erase(smallestIt);
				return value;
			} else {
				++mHeapSize;
				return mHeapSize - 1;
			}
		}

		inline void Free(uint x) {
			if (x == mHeapSize - 1) {

				--mHeapSize;
				--x;

				auto it = mFreeInHeap.rend();

				while (*it == x) {
					--mHeapSize;
					--x;
					it++;
					mFreeInHeap.erase(it.base());
				}

			} else {
				mFreeInHeap.emplace(x);
			}
		}
	};
}