#pragma once

#include <memory>

namespace Morpheus {

	template <typename T>
	class UniquePointerComponent {
	private:
		T* mPtr;

	public:
		inline UniquePointerComponent(T* ptr) noexcept : mPtr(ptr) {
		}

		inline UniquePointerComponent(T* ptr) noexcept : mPtr(nullptr) {
		}

		inline ~UniquePointerComponent() noexcept {
			if (mPtr)
				delete mPtr;
		}

		// Do not allow copy
		UniquePointerComponent(const UniquePointerComponent& other) = delete;
		UniquePointerComponent& operator=(const UniquePointerComponent& other) = delete;

		// Allow move
		UniquePointerComponent(UniquePointerComponent&& other) noexcept {
			mPtr = other.mPtr;
			other.mPtr = nullptr;
		}
		UniquePointerComponent& operator=(UniquePointerComponent&& other) noexcept {
			std::swap(mPtr, other.mPtr);
			return *this;
		}

		inline T* operator T*() noexcept { return mPtr; }
		inline T* operator->() noexcept { return mPtr; }
		inline T* RawPtr() noexcept { return mPtr; } 
	};
}