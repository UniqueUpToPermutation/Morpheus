#include <Engine/Resources/Texture.hpp>
#include <Engine/Renderer.hpp>

namespace Morpheus {
	MaterialDesc MaterialDescFuture::Get() const {
		MaterialDesc desc;

		desc.mType = mType;

		if (mAlbedo)
			desc.mAlbedo = mAlbedo.Get();
		if (mNormal)
			desc.mNormal = mNormal.Get();
		if (mRoughness)
			desc.mRoughness = mRoughness.Get();
		if (mMetallic)
			desc.mMetallic = mMetallic.Get();
		if (mDisplacement)
			desc.mDisplacement = mDisplacement.Get();

		desc.mAlbedoFactor = mAlbedoFactor;
		desc.mRoughnessFactor = mRoughnessFactor;
		desc.mMetallicFactor = mMetallicFactor;
		desc.mDisplacementFactor = mDisplacementFactor;

		return desc;
	}

	bool MaterialDescFuture::IsAvailable() const {
		return mAlbedo.IsAvailable() &&
			mNormal.IsAvailable() &&
			mRoughness.IsAvailable() &&
			mMetallic.IsAvailable() &&
			mDisplacement.IsAvailable();
	}

	void MaterialDescFuture::Connect(TaskNodeInLock& lock) {
		if (mAlbedo)
			lock.Connect(mAlbedo.Out());
		if (mNormal)
			lock.Connect(mNormal.Out());
		if (mRoughness)
			lock.Connect(mRoughness.Out());
		if (mMetallic)
			lock.Connect(mMetallic.Out());
		if (mDisplacement)
			lock.Connect(mDisplacement.Out());
	}
}