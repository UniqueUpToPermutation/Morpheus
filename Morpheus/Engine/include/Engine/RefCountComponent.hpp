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

		inline ~RefCountComponent() noexcept {
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

		inline T* operator->() noexcept  {
			return mPtr;
		}

		inline T* RawPtr() noexcept {
			return mPtr;
		}
	};
}