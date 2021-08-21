#include <Engine/Resources/Texture.hpp>
#include <Engine/Renderer.hpp>

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
}