#include <Engine/HdriToCubemap.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/LightProbeProcessor.hpp>
#include <Engine/Resources/Shader.hpp>

#include "GraphicsUtilities.h"
#include "MapHelper.hpp"

namespace Morpheus {
	ResourceTask<HDRIToCubemapShaders> HDRIToCubemapShaders::Load(
		DG::IRenderDevice* device,
		bool bConvertSRGBToLinear,
		IVirtualFileSystem* fileSystem) {

		Promise<HDRIToCubemapShaders> promise;
		Future<HDRIToCubemapShaders> future(promise);

		struct {
			Future<DG::IShader*> mVS;
			Future<DG::IShader*> mPS;
		} data;

		Task task([device, promise = std::move(promise), data, 
			bConvertSRGBToLinear, fileSystem]
			(const TaskParams& e) mutable {
			if (e.mTask->BeginSubTask()) {

				ShaderPreprocessorConfig config;
				if (bConvertSRGBToLinear)
					config.mDefines["TRANSFORM_SRGB_TO_LINEAR"] = "1";
				else 
					config.mDefines["TRANSFORM_SRGB_TO_LINEAR"] = "0";

				LoadParams<RawShader> vsParams("internal/CubemapFace.vsh",
					DG::SHADER_TYPE_VERTEX,
					"Cubemap Face Vertex Shader",
					config,
					"main");

				LoadParams<RawShader> psParams("internal/HdriToCubemap.psh",
					DG::SHADER_TYPE_PIXEL,
					"HDRI Convert Pixel Shader",
					config,
					"main");

				auto vsTask = LoadShader(device, vsParams, fileSystem);
				auto psTask = LoadShader(device, psParams, fileSystem);

				data.mVS = e.mQueue->AdoptAndTrigger(std::move(vsTask));
				data.mPS = e.mQueue->AdoptAndTrigger(std::move(psTask));

				e.mTask->EndSubTask();

				if (e.mTask->In().Lock()
					.Connect(data.mVS.Out())
					.Connect(data.mPS.Out())
					.ShouldWait())
					return TaskResult::WAITING;
			}

			HDRIToCubemapShaders shaders;
			shaders.mVS.Adopt(data.mVS.Get());
			shaders.mPS.Adopt(data.mPS.Get());

			promise.Set(std::move(shaders), e.mQueue);

			return TaskResult::FINISHED;
		}, "Load HDRI To Cubemap Shaders", TaskType::FILE_IO);

		ResourceTask<HDRIToCubemapShaders> result;
		result.mTask = std::move(task);
		result.mFuture = std::move(future);

		return result;
	}

	HDRIToCubemapConverter::HDRIToCubemapConverter(DG::IRenderDevice* device, 
		const HDRIToCubemapShaders& shaders,
		DG::TEXTURE_FORMAT cubemapFormat) {

		DG::CreateUniformBuffer(device, sizeof(PrecomputeEnvMapAttribs),
			"Light Probe Processor Constants Buffer", 
			mTransformConstantBuffer.Ref());
		
		mCubemapFormat = cubemapFormat;

		DG::SamplerDesc SamLinearClampDesc
		{
			DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, 
			DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
		};

		{
			// Create Irradiance Pipeline
			DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
			DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
			DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

			PSODesc.Name         = "HDRI To Cubemap Pipeline";
			PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

			GraphicsPipeline.NumRenderTargets             = 1;
			GraphicsPipeline.RTVFormats[0]                = cubemapFormat;
			GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_NONE;
			GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

			PSOCreateInfo.pVS = shaders.mVS;
			PSOCreateInfo.pPS = shaders.mPS;

			PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
			// clang-format off
			DG::ShaderResourceVariableDesc Vars[] = 
			{
				{DG::SHADER_TYPE_PIXEL, "g_HDRI", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
			};
			// clang-format on
			PSODesc.ResourceLayout.NumVariables = _countof(Vars);
			PSODesc.ResourceLayout.Variables    = Vars;

			// clang-format off
			DG::ImmutableSamplerDesc ImtblSamplers[] =
			{
				{DG::SHADER_TYPE_PIXEL, "g_HDRI_sampler", SamLinearClampDesc}
			};
			// clang-format on
			PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
			PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;

			device->CreateGraphicsPipelineState(PSOCreateInfo, mPipelineState.Ref());
			mPipelineState->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "mTransform")->Set(mTransformConstantBuffer);
			mPipelineState->CreateShaderResourceBinding(mSRB.Ref(), true);
		}
	}

	void HDRIToCubemapConverter::Convert(DG::IDeviceContext* context, 
		DG::ITextureView* hdri,
		DG::ITexture* outputCubemap) {
		if (!mPipelineState) {
			throw std::runtime_error("Initialize has not been called!");
		}
		if (outputCubemap->GetDesc().Format != mCubemapFormat) {
			throw std::runtime_error("Output cubemap does not have correct format!");
		}

		// clang-format off
		const std::array<DG::float4x4, 6> Matrices =
		{
	/* +X */ DG::float4x4::RotationY(+DG::PI_F / 2.f),
	/* -X */ DG::float4x4::RotationY(-DG::PI_F / 2.f),
	/* +Y */ DG::float4x4::RotationX(-DG::PI_F / 2.f),
	/* -Y */ DG::float4x4::RotationX(+DG::PI_F / 2.f),
	/* +Z */ DG::float4x4::Identity(),
	/* -Z */ DG::float4x4::RotationY(DG::PI_F)
		};
		// clang-format on

		context->SetPipelineState(mPipelineState);
		mSRB->GetVariableByName(DG::SHADER_TYPE_PIXEL, "g_HDRI")->Set(hdri);
		context->CommitShaderResources(mSRB, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		auto*       pIrradianceCube    = outputCubemap;
		const auto& IrradianceCubeDesc = pIrradianceCube->GetDesc();
		for (uint mip = 0; mip < IrradianceCubeDesc.MipLevels; ++mip)
		{
			for (uint face = 0; face < 6; ++face)
			{
				DG::TextureViewDesc RTVDesc(DG::TEXTURE_VIEW_RENDER_TARGET, DG::RESOURCE_DIM_TEX_2D_ARRAY);
				RTVDesc.Name            = "RTV for HDRI Cubemap";
				RTVDesc.MostDetailedMip = mip;
				RTVDesc.FirstArraySlice = face;
				RTVDesc.NumArraySlices  = 1;
				Handle<DG::ITextureView> pRTV;
				pIrradianceCube->CreateView(RTVDesc, pRTV.Ref());
				DG::ITextureView* ppRTVs[] = {pRTV};
				context->SetRenderTargets(_countof(ppRTVs), ppRTVs, nullptr, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
				{
					DG::MapHelper<PrecomputeEnvMapAttribs> Attribs(context, mTransformConstantBuffer, 
						DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
					Attribs->Rotation = Matrices[face];
				}
				DG::DrawAttribs drawAttrs(4, DG::DRAW_FLAG_VERIFY_ALL);
				context->Draw(drawAttrs);
			}
		}

		// clang-format off
		DG::StateTransitionDesc Barriers[] = 
		{
			{outputCubemap, DG::RESOURCE_STATE_UNKNOWN, DG::RESOURCE_STATE_SHADER_RESOURCE, true}
		};
		// clang-format on
		context->TransitionResourceStates(_countof(Barriers), Barriers);
	}

	DG::ITexture* HDRIToCubemapConverter::Convert(DG::IRenderDevice* device,
		DG::IDeviceContext* context,
		DG::ITextureView* hdri,
		uint size,
		bool bGenerateMips) {

		DG::TextureDesc desc;
		desc.BindFlags = DG::BIND_RENDER_TARGET | DG::BIND_SHADER_RESOURCE;
		desc.Width = size;
		desc.Height = size;
		desc.MipLevels = bGenerateMips ? 0 : 1;
		desc.ArraySize = 6;
		desc.Type = DG::RESOURCE_DIM_TEX_CUBE;
		desc.Usage = DG::USAGE_DEFAULT;
		desc.Name = "HDRI Generated Cubemap";
		desc.Format = mCubemapFormat;
		
		DG::ITexture* result = nullptr;
		device->CreateTexture(desc, nullptr, &result);

		Convert(context, hdri, result);

		return result;
	}
}