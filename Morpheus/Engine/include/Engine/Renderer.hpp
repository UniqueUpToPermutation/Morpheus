#pragma once

#include <Engine/ThreadPool.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Resources/Resource.hpp>

namespace Morpheus {

	class IRenderer;

	enum class MaterialType {
		COOK_TORRENCE,
		LAMBERT,
		CUSTOM
	};

	struct MaterialDesc {
		MaterialType mType = MaterialType::COOK_TORRENCE;

		Handle<Texture> mAlbedo;
		Handle<Texture> mNormal;
		Handle<Texture> mRoughness;
		Handle<Texture> mMetallic;
		Handle<Texture> mDisplacement;

		DG::float4 mAlbedoFactor 	= DG::float4(1.0f, 1.0f, 1.0f, 1.0f);
		float mRoughnessFactor 		= 1.0f;
		float mMetallicFactor 		= 1.0f;
		float mDisplacementFactor 	= 1.0f;
	};

	struct MaterialDescFuture : public IVirtualTaskNodeOut {
		MaterialType mType = MaterialType::COOK_TORRENCE;

		Future<Texture*> mAlbedo;
		Future<Texture*> mNormal;
		Future<Texture*> mRoughness;
		Future<Texture*> mMetallic;
		Future<Texture*> mDisplacement;

		DG::float4 mAlbedoFactor 	= DG::float4(1.0f, 1.0f, 1.0f, 1.0f);
		float mRoughnessFactor 		= 1.0f;
		float mMetallicFactor 		= 1.0f;
		float mDisplacementFactor 	= 1.0f;

		MaterialDesc Get() const;
		bool IsAvailable() const;
		void Connect(TaskNodeInLock& lock) override;

		inline IVirtualTaskNodeOut& Out() {
			return *this;
		}
	};

	class Material {
	private:
		IRenderer* mRenderer;
		MaterialId mId;

	public:
		inline Material() : mRenderer(nullptr), mId(NullMaterialId) {
		}

		inline Material(IRenderer* renderer, MaterialId id);
		inline ~Material();

		inline Material(const Material& mat);
		inline Material(Material&& mat);

		inline MaterialId Id() const {
			return mId;
		}

		inline operator MaterialId() const {
			return mId;
		}

		inline bool operator==(const Material& other) const {
			return mId == other.mId;
		}

		inline Material& operator=(const Material& mat);
		inline Material& operator=(Material&& mat);
	};

	class IRenderer {
	public:		
		// Must be called from main thread!
		virtual MaterialId CreateUnmanagedMaterial(const MaterialDesc& desc) = 0;

		// Thread Safe
		virtual void AddMaterialRef(MaterialId id) = 0;
		// Thread Safe
		virtual void ReleaseMaterial(MaterialId id) = 0;

		// Must be called from main thread!
		inline Material CreateMaterial(const MaterialDesc& desc) {
			MaterialId id = CreateUnmanagedMaterial(desc);
			Material mat(this, id);
			ReleaseMaterial(id);
			return mat;
		}
	};

	Material::Material(IRenderer* renderer, MaterialId id) :
		mRenderer(renderer), mId(id) {
		renderer->AddMaterialRef(id);
	}

	Material::~Material() {
		if (mId != NullMaterialId) {
			mRenderer->ReleaseMaterial(mId);
		}
	}

	Material::Material(const Material& mat) :
		mRenderer(mat.mRenderer), mId(mat.mId) {
		mRenderer->AddMaterialRef(mId);
	}

	Material::Material(Material&& mat) : 
		mRenderer(mat.mRenderer),
		mId(mat.mId) {
		mat.mId = NullMaterialId;
	}

	Material& Material::operator=(const Material& mat) {
		mRenderer = mat.mRenderer;
		mId = mat.mId;
		mRenderer->AddMaterialRef(mId);
		return *this;
	}

	Material& Material::operator=(Material&& mat) {
		std::swap(mat.mRenderer, mRenderer);
		std::swap(mat.mId, mId);
		return *this;
	}
}