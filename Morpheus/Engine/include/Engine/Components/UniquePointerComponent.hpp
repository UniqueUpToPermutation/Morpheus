#pragma once

#include <memory>

namespace Morpheus {

	template <class, template <class> class>
	struct is_instance : public std::false_type {};

	template <class T, template <class> class U>
	struct is_instance<U<T>, U> : public std::true_type {};

	template <class T>
	class UniquePointerComponent {
	private:
		T* mPtr;

	public:
		inline UniquePointerComponent(T* ptr) noexcept : mPtr(ptr) {
		}

		inline UniquePointerComponent() noexcept : mPtr(nullptr) {
		}

		inline operator T*() noexcept { return mPtr; }
		inline T* operator->() noexcept { return mPtr; }
		inline T* RawPtr() noexcept { return mPtr; } 
	};

	template <class T>
	constexpr bool IsUniquePointerComponent() {
		return is_instance<T, UniquePointerComponent>{};
	}
}