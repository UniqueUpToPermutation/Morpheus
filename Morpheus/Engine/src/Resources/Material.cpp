#include <Engine/Resources/Material.hpp>
#include <Engine/Resources/Texture.hpp>

namespace Morpheus {
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

	void Material::BinarySerialize(std::ostream& output) const {

	}

	void Material::BinaryDeserialize(std::istream& input) {

	}

	void Material::BinarySerializeReference(
		const std::filesystem::path& workingPath, 
		cereal::PortableBinaryOutputArchive& output) const {

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
}