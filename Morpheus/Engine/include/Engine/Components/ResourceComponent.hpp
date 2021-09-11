#pragma once

#include <Engine/Resources/Resource.hpp>

namespace Morpheus {
	template <typename T>
	struct ResourceComponent {
		Handle<T> mResource;

		ResourceComponent() = default;
		inline ResourceComponent(Handle<T> h) : mResource(h) {
		}

		inline T* operator* () {
			return mResource.Ptr();
		}

		inline void BinarySerializeReference(
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryOutputArchive& arr) {
			mResource->BinarySerializeReference(workingPath, arr);
		}
	};
}