#include <Engine/Resource.hpp>
#include <Engine/ResourceManager.hpp>

namespace Morpheus {
	void Resource::Release() {
		--mRefCount;
		if (mRefCount == 0) {
			mManager->RequestUnload(this);
		}
	}
}