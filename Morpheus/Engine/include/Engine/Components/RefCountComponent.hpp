#pragma once

namespace Morpheus {
	template <typename T>
	class RefCountComponent {
	private:
		T* mPtr;

	public:
		inline RefCountComponent(T* ptr) noexcept :
			mPtr(ptr) {
			mPtr->AddRef();
		}

		inline RefCountComponent() noexcept :
			mPtr(nullptr) {
		}

		inline ~RefCountComponent() noexcept {
			if (mPtr)
				mPtr->Release();
		}

		inline RefCountComponent(const RefCountComponent& other) noexcept : mPtr(other.mPtr) {
			mPtr->AddRef();
		}

		inline RefCountComponent& operator=(const RefCountComponent& other) noexcept {
			mPtr->Release();
			mPtr = other.mPtr;
			mPtr->AddRef(); 
			return *this;
		}

		RefCountComponent(RefCountComponent&& other) {
			mPtr = other.mPtr;
			other.mPtr = nullptr;
		}

		RefCountComponent& operator=(RefCountComponent&& other) {
			std::swap(mPtr, other.mPtr);
			return *this;
		}

		inline T* operator->() noexcept  {
			return mPtr;
		}

		inline T* RawPtr() noexcept {
			return mPtr;
		}
	};
}