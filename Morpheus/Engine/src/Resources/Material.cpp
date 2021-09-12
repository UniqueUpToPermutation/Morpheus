#include <Engine/Resources/Material.hpp>
#include <Engine/Resources/Texture.hpp>
#include <Engine/Resources/FrameIO.hpp>

#include <cereal/archives/portable_binary.hpp>

using namespace entt;

namespace Morpheus {
	template <typename Archive>
	void serialize(Archive& arr, MaterialDesc::Params& params) {
		arr(params.mAlbedoFactor);
		arr(params.mDisplacementFactor);
		arr(params.mMetallicFactor);
		arr(params.mRoughnessFactor);
		arr(params.mType);
	}

	UniqueFuture<MaterialDesc> MaterialDesc::CreateFuture(
		Future<Handle<Texture>> albedo,
		Future<Handle<Texture>> normal,
		Future<Handle<Texture>> roughness,
		Future<Handle<Texture>> metallic,
		Future<Handle<Texture>> displacement,
		const Params& params) {

		Promise<MaterialDesc> output;
		
		FunctionPrototype<
			Future<Handle<Texture>>,
			Future<Handle<Texture>>,
			Future<Handle<Texture>>,
			Future<Handle<Texture>>,
			Future<Handle<Texture>>,
			Promise<MaterialDesc>>
			prototype([params](
				const TaskParams& e,
				Future<Handle<Texture>> albedo,
				Future<Handle<Texture>> normal,
				Future<Handle<Texture>> roughness,
				Future<Handle<Texture>> metallic,
				Future<Handle<Texture>> displacement,
				Promise<MaterialDesc> output) {
			
			MaterialDesc desc;
			if (albedo)
				desc.mResources.mAlbedo = albedo.Get();
			if (normal)
				desc.mResources.mNormal = normal.Get();
			if (roughness)
				desc.mResources.mRoughness = roughness.Get();
			if (metallic)
				desc.mResources.mMetallic = metallic.Get();
			if (displacement)
				desc.mResources.mDisplacement = displacement.Get();

			desc.mParams = params;

			output = std::move(desc);
		});

		prototype(albedo, normal, roughness, metallic, displacement, output)
			.SetName("Create MaterialDesc Future");

		return output;
	}

	entt::meta_type Material::GetType() const {
		return entt::resolve<Material>();
	}

	entt::meta_any Material::GetSourceMeta() const {
		return nullptr;
	}

	std::filesystem::path Material::GetPath() const {
		return "";
	}

	void Material::BinarySerialize(std::ostream& output, IDependencyResolver* dependencies) {
		cereal::PortableBinaryOutputArchive arr(output);

		arr(mDesc.mParams);

		ResourceId albedo = dependencies->AddDependency(
			mDesc.mResources.mAlbedo.DownCast<IResource>());
		ResourceId normal = dependencies->AddDependency(
			mDesc.mResources.mNormal.DownCast<IResource>());
		ResourceId metallic = dependencies->AddDependency(
			mDesc.mResources.mMetallic.DownCast<IResource>());
		ResourceId roughness = dependencies->AddDependency(
			mDesc.mResources.mRoughness.DownCast<IResource>());
		ResourceId displacement = dependencies->AddDependency(
			mDesc.mResources.mDisplacement.DownCast<IResource>());

		arr(albedo);
		arr(normal);
		arr(metallic);
		arr(roughness);
		arr(displacement);
	}

	void Material::BinaryDeserialize(std::istream& input, const IDependencyResolver* dependencies) {
		cereal::PortableBinaryInputArchive arr(input);

		arr(mDesc.mParams);

		ResourceId albedo;
		ResourceId normal;
		ResourceId metallic;
		ResourceId roughness;
		ResourceId displacement;

		arr(albedo);
		arr(normal);
		arr(metallic);
		arr(roughness);
		arr(displacement);

		mDesc.mResources.mAlbedo = dependencies->GetDependency(albedo).TryCast<Texture>();
		mDesc.mResources.mNormal = dependencies->GetDependency(normal).TryCast<Texture>();
		mDesc.mResources.mMetallic = dependencies->GetDependency(metallic).TryCast<Texture>();
		mDesc.mResources.mRoughness = dependencies->GetDependency(roughness).TryCast<Texture>();
		mDesc.mResources.mDisplacement = dependencies->GetDependency(displacement).TryCast<Texture>();
	}

	void Material::BinarySerializeReference(
		const std::filesystem::path& workingPath, 
		cereal::PortableBinaryOutputArchive& output) {
	}

	void Material::BinaryDeserializeReference(
		const std::filesystem::path& workingPath,
		cereal::PortableBinaryInputArchive& input) {
	}

	BarrierOut Material::MoveAsync(Device device, Context context) {
		return Barrier();
	}

	Handle<IResource> Material::MoveIntoHandle() {
		return Handle<Material>(Duplicate()).template DownCast<IResource>();
	}

	Material::Material(const MaterialDesc& desc) : mDesc(desc) {
	}

	Material Material::Duplicate() {
		return Material(mDesc);
	}

	const MaterialDesc& Material::GetDesc() const {
		return mDesc;
	}

	void Material::RegisterMetaData() {
		meta<Material>()
			.type("Material"_hs)
			.base<IResource>();

		MakeSerializableResourceType<Material>();
	}
}