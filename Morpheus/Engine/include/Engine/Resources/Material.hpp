#pragma once 

#include <memory>

#include <Engine/Resources/Resource.hpp>

namespace Morpheus {
	enum class MaterialType {
		COOK_TORRENCE,
		LAMBERT,
		CUSTOM
	};

	struct MaterialDesc {
		struct Params {
			MaterialType mType = MaterialType::COOK_TORRENCE;
			DG::float4 mAlbedoFactor 	= DG::float4(1.0f, 1.0f, 1.0f, 1.0f);
			float mRoughnessFactor 		= 1.0f;
			float mMetallicFactor 		= 1.0f;
			float mDisplacementFactor 	= 1.0f;
		} mParams;

		struct Resources {
			Handle<Texture> mAlbedo;
			Handle<Texture> mNormal;
			Handle<Texture> mRoughness;
			Handle<Texture> mMetallic;
			Handle<Texture> mDisplacement;
		} mResources;
		
		static UniqueFuture<MaterialDesc> CreateFuture(
			Future<Handle<Texture>> albedo,
			Future<Handle<Texture>> normal,
			Future<Handle<Texture>> roughness,
			Future<Handle<Texture>> metallic,
			Future<Handle<Texture>> displacement,
			const Params& params);
	};

	class Material : public IResource {
	private:
		MaterialDesc mDesc;

	public:
		inline Material() {
		}
		
		Material(const MaterialDesc& desc);
		Material(Material&&) = default;
		Material& operator=(Material&&) = default;
		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;

		Material Duplicate();
		const MaterialDesc& GetDesc() const;
		entt::meta_type GetType() const;
		entt::meta_any GetSourceMeta() const;
		std::filesystem::path GetPath() const;
		void BinarySerialize(std::ostream& output, IDependencyResolver* dependencies);
		void BinaryDeserialize(std::istream& input, const IDependencyResolver* dependencies);
		void BinarySerializeReference(
			const std::filesystem::path& workingPath, 
			cereal::PortableBinaryOutputArchive& output);
		void BinaryDeserializeReference(
			const std::filesystem::path& workingPath,
			cereal::PortableBinaryInputArchive& input);
		BarrierOut MoveAsync(Device device, Context context = Context());
		Handle<IResource> MoveIntoHandle();

		static void RegisterMetaData();
	};
}