#include <Engine/Brdf.hpp>

#include <Engine/PipelineResource.hpp>
#include <Engine/TextureResource.hpp>

namespace Morpheus {
	CookTorranceLUT::CookTorranceLUT(DG::IRenderDevice* device, 
		uint NdotVSamples, 
		uint RoughnessSamples) {

		mLut = nullptr;

		DG::TextureDesc desc;
		desc.BindFlags = DG::BIND_SHADER_RESOURCE | DG::BIND_RENDER_TARGET;
		desc.Width = NdotVSamples;
		desc.Height = RoughnessSamples;
		desc.CPUAccessFlags = DG::CPU_ACCESS_NONE;
		desc.Format = DG::TEX_FORMAT_RG16_FLOAT;
		desc.MipLevels = 1;
		desc.Name      = "GLTF BRDF Look-up texture";
   	 	desc.Type      = DG::RESOURCE_DIM_TEX_2D;
    	desc.Usage     = DG::USAGE_DEFAULT;

		device->CreateTexture(desc, nullptr, &mLut);
	}

	CookTorranceLUT::~CookTorranceLUT() {
		mLut->Release();
	}

	void CookTorranceLUT::Compute(ResourceManager* resourceManager,
		DG::IDeviceContext* context, 
		DG::IRenderDevice* device,
		uint Samples) {

		LoadParams<PipelineResource> params;
		params.mSource = "internal/PrecomputeBRDF.json";
		params.mOverrides.mDefines["NUM_SAMPLES"] = std::to_string(Samples);

		auto pipeline = resourceManager->Load<PipelineResource>(params);

		context->SetPipelineState(pipeline->GetState());

		DG::ITextureView* pRTVs[] = {mLut->GetDefaultView(DG::TEXTURE_VIEW_RENDER_TARGET)};
		context->SetRenderTargets(1, pRTVs, nullptr, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		DG::DrawAttribs attrs(3, DG::DRAW_FLAG_VERIFY_ALL);
		context->Draw(attrs);

		// clang-format off
		DG::StateTransitionDesc Barriers[] =
		{
			{mLut, DG::RESOURCE_STATE_UNKNOWN, DG::RESOURCE_STATE_SHADER_RESOURCE, true}
		};
		// clang-format on
		context->TransitionResourceStates(_countof(Barriers), Barriers);

		pipeline->Release();
	}

	void CookTorranceLUT::SavePng(const std::string& path, DG::IDeviceContext* context, DG::IRenderDevice* device) {
		Morpheus::SavePng(mLut, path, context, device);
	}

	void CookTorranceLUT::SaveGli(const std::string& path, DG::IDeviceContext* context, DG::IRenderDevice* device) {
		Morpheus::SaveGli(mLut, path, context, device);
	}
}