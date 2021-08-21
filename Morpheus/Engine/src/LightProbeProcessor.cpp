#include <Engine/LightProbeProcessor.hpp>

#include <Engine/Resources/Texture.hpp>
#include <Engine/Resources/Shader.hpp>

#include "GraphicsUtilities.h"
#include "MapHelper.hpp"

namespace Morpheus {

	Future<LightProbeProcessorShaders> LightProbeProcessorShaders::Load(
		DG::IRenderDevice* device,
		const LightProbeProcessorConfig& config,
		IVirtualFileSystem* fileSystem) {

		struct {
			Future<Handle<DG::IShader>> mVS;
			Future<Handle<DG::IShader>> mPS;
			Future<Handle<DG::IShader>> mCS;
		} data;

		ShaderPreprocessorConfig prefilterEnvConfig;
		ShaderPreprocessorConfig irradianceSHConfig;

		prefilterEnvConfig.mDefines["OPTIMIZE_SAMPLES"] = std::to_string((int)config.bEnvMapOptimizeSamples);
		irradianceSHConfig.mDefines["SAMPLE_COUNT"] = std::to_string(config.mIrradianceSHSamples);

		LoadParams<RawShader> vsParams("internal/CubemapFace.vsh",
			DG::SHADER_TYPE_VERTEX,
			"Cubemap Face Vertex Shader");

		LoadParams<RawShader> envPsParams("internal/PrefilterEnvironment.psh",
			DG::SHADER_TYPE_PIXEL,
			"Compute Environment Pixel Shader",
			prefilterEnvConfig);

		LoadParams<RawShader> irrSHParams("internal/ComputeIrradianceSH.csh",
			DG::SHADER_TYPE_COMPUTE,
			"Compute Irradiance SH Compute Shader",
			irradianceSHConfig);

		data.mVS = LoadShaderHandle(device, vsParams, fileSystem);
		data.mPS = LoadShaderHandle(device, envPsParams, fileSystem);
		data.mCS = LoadShaderHandle(device, irrSHParams, fileSystem);

		FunctionPrototype<
			Future<Handle<DG::IShader>>,
			Future<Handle<DG::IShader>>,
			Future<Handle<DG::IShader>>,
			Promise<LightProbeProcessorShaders>>
			prototype([](const TaskParams& e,
				Future<Handle<DG::IShader>> vs,
				Future<Handle<DG::IShader>> ps,
				Future<Handle<DG::IShader>> cs,
				Promise<LightProbeProcessorShaders> result) {

			LightProbeProcessorShaders shaders;
			shaders.mPrefilterEnvVS = vs.Get();
			shaders.mPrefilterEnvPS = ps.Get();
			shaders.mSHShaderCS = cs.Get();
		
			result = std::move(shaders);
		});

		Promise<LightProbeProcessorShaders> result;

		prototype(data.mVS, data.mPS, data.mCS, result)
			.SetName("Create Light Probe Processor Shader Struct");

		return result;
	}

	void CookTorranceLUT::Compute(DG::IRenderDevice* device,
		DG::IDeviceContext* context,
		uint surfaceAngleSamples, 
		uint roughnessSamples,
		uint integrationSamples) {

		DG::TextureDesc desc;
		desc.BindFlags = DG::BIND_SHADER_RESOURCE | DG::BIND_RENDER_TARGET;
		desc.Width = surfaceAngleSamples;
		desc.Height = roughnessSamples;
		desc.CPUAccessFlags = DG::CPU_ACCESS_NONE;
		desc.Format = DG::TEX_FORMAT_RG16_FLOAT;
		desc.MipLevels = 1;
		desc.Name      = "GLTF BRDF Look-up texture";
   	 	desc.Type      = DG::RESOURCE_DIM_TEX_2D;
    	desc.Usage     = DG::USAGE_DEFAULT;

		DG::ITexture* lut = nullptr;
		device->CreateTexture(desc, nullptr, &lut);
		mLut = Texture(device, lut);

		ShaderPreprocessorConfig overrides;
		overrides.mDefines["NUM_SAMPLES"] = std::to_string(integrationSamples);

		LoadParams<RawShader> vsParams(
			"internal/FullscreenTriangle.vsh",
			DG::SHADER_TYPE_VERTEX,
			"Fullscreen Triangle",
			overrides,
			"main"
		);

		LoadParams<RawShader> psParams(
			"internal/PrecomputeBRDF.psh",
			DG::SHADER_TYPE_PIXEL,
			"Fullscreen Triangle",
			overrides,
			"main"
		);

		auto vsResource = CompileEmbeddedShader(device, vsParams);
		auto psResource = CompileEmbeddedShader(device, psParams);

		DG::GraphicsPipelineStateCreateInfo psoInfo;
		psoInfo.PSODesc.Name = "Precompute BRDF PSO";
		psoInfo.PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

		psoInfo.GraphicsPipeline.NumRenderTargets = 1;
		psoInfo.GraphicsPipeline.RTVFormats[0] = DG::TEX_FORMAT_RG16_FLOAT;
		psoInfo.GraphicsPipeline.PrimitiveTopology = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		psoInfo.GraphicsPipeline.RasterizerDesc.CullMode = DG::CULL_MODE_NONE;
		psoInfo.GraphicsPipeline.SmplDesc.Count = 1;
		psoInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
		
		psoInfo.pVS = vsResource;
		psoInfo.pPS = psResource;

		DG::IPipelineState* pipelineState = nullptr;
		device->CreateGraphicsPipelineState(psoInfo, &pipelineState);
		context->SetPipelineState(pipelineState);

		DG::ITextureView* pRTVs[] = {mLut.GetRenderTargetView()};
		context->SetRenderTargets(1, pRTVs, nullptr, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		DG::DrawAttribs attrs(3, DG::DRAW_FLAG_VERIFY_ALL);
		context->Draw(attrs);

		// clang-format off
		DG::StateTransitionDesc Barriers[] =
		{
			{mLut.GetRasterTexture(), DG::RESOURCE_STATE_UNKNOWN, DG::RESOURCE_STATE_SHADER_RESOURCE, true}
		};
		// clang-format on
		context->TransitionResourceStates(_countof(Barriers), Barriers);

		pipelineState->Release();
		vsResource->Release();
		psResource->Release();
	}

	void CookTorranceLUT::SavePng(const std::string& path, 
		DG::IDeviceContext* context, 
		DG::IRenderDevice* device) {
		
		Texture texture = mLut.To(Device::CPU(), context);
		texture.SavePng(path);
	}

	LightProbeProcessor::LightProbeProcessor(DG::IRenderDevice* device,
		const LightProbeProcessorShaders& shaders,
		const LightProbeProcessorConfig& config) : 

		mPrefilterEnvPipeline(nullptr), 
		mPrefilterEnvSRB(nullptr),
		mSHIrradianceSRB(nullptr),
		mTransformConstantBuffer(nullptr) {

		DG::CreateUniformBuffer(device, sizeof(PrecomputeEnvMapAttribs),
			"Light Probe Processor Constants Buffer", 
			mTransformConstantBuffer.Ref());

		mConfig = config;

		DG::SamplerDesc SamLinearClampDesc
		{
			DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, 
			DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
		};

		{
			// Create Environment Pipeline
			DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
			DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
			DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

			PSODesc.Name         = "Prefilter environment map PSO";
			PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

			GraphicsPipeline.NumRenderTargets             = 1;
			GraphicsPipeline.RTVFormats[0]                = mConfig.mPrefilteredEnvFormat;
			GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_NONE;
			GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

			PSOCreateInfo.pVS = shaders.mPrefilterEnvVS.Ptr();
			PSOCreateInfo.pPS = shaders.mPrefilterEnvPS.Ptr();

			PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
			// clang-format off
			DG::ShaderResourceVariableDesc Vars[] = 
			{
				{DG::SHADER_TYPE_PIXEL, "g_EnvironmentMap", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
			};
			// clang-format on
			PSODesc.ResourceLayout.NumVariables = _countof(Vars);
			PSODesc.ResourceLayout.Variables    = Vars;

			// clang-format off
			DG::ImmutableSamplerDesc ImtblSamplers[] =
			{
				{DG::SHADER_TYPE_PIXEL, "g_EnvironmentMap_sampler", SamLinearClampDesc}
			};
			// clang-format on
			PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
			PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;

			device->CreateGraphicsPipelineState(PSOCreateInfo, mPrefilterEnvPipeline.Ref());
			mPrefilterEnvPipeline->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "mTransform")->Set(mTransformConstantBuffer);
			auto filterAttribs = mPrefilterEnvPipeline->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "FilterAttribs");
			if (filterAttribs)
				filterAttribs->Set(mTransformConstantBuffer);
			mPrefilterEnvPipeline->CreateShaderResourceBinding(mPrefilterEnvSRB.Ref(), true);
		}

		{
			DG::ComputePipelineStateCreateInfo PSOCreateInfo;
			DG::PipelineStateDesc&             PSODesc = PSOCreateInfo.PSODesc;

			// This is a compute pipeline
			PSODesc.PipelineType = DG::PIPELINE_TYPE_COMPUTE;

			PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

			DG::ShaderResourceVariableDesc Vars[] = 
			{
				{DG::SHADER_TYPE_COMPUTE, "mEnvironmentMap", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
			};

			// clang-format on
			PSODesc.ResourceLayout.NumVariables = _countof(Vars);
			PSODesc.ResourceLayout.Variables    = Vars;
			
			// clang-format off
			DG::ImmutableSamplerDesc ImtblSamplers[] =
			{
				{DG::SHADER_TYPE_COMPUTE, "mEnvironmentMap_sampler", SamLinearClampDesc}
			};

			// clang-format on
			PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
			PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;

			PSODesc.Name      = "Irradiance SH PSO";
			PSOCreateInfo.pCS = shaders.mSHShaderCS.Ptr();

			device->CreateComputePipelineState(PSOCreateInfo, mSHIrradiancePipeline.Ref());
			mSHIrradiancePipeline->CreateShaderResourceBinding(mSHIrradianceSRB.Ref(), true);
		}
	}

	void LightProbeProcessor::ComputeIrradiance(
		DG::IDeviceContext* context,
		DG::ITextureView* incommingEnvironmentSRV,
		DG::IBufferView* outputBufferView) {

		if (!mSHIrradiancePipeline) {
			throw std::runtime_error("Initialize has not been called!");
		}

		context->SetPipelineState(mSHIrradiancePipeline);

		mSHIrradianceSRB->GetVariableByName(DG::SHADER_TYPE_COMPUTE, "mEnvironmentMap")->Set(incommingEnvironmentSRV);
		mSHIrradianceSRB->GetVariableByName(DG::SHADER_TYPE_COMPUTE, "mCoeffsOut")->Set(outputBufferView);

		context->CommitShaderResources(mSHIrradianceSRB, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		
		DG::DispatchComputeAttribs attribs;		
		context->DispatchCompute(attribs);

		// clang-format off
		DG::StateTransitionDesc Barriers[] = 
		{
			{outputBufferView->GetBuffer(), DG::RESOURCE_STATE_UNKNOWN, DG::RESOURCE_STATE_SHADER_RESOURCE, true}
		};
		// clang-format on
		context->TransitionResourceStates(_countof(Barriers), Barriers);
	}

	DG::IBuffer* LightProbeProcessor::ComputeIrradiance(
		DG::IRenderDevice* device,
		DG::IDeviceContext* context,
		DG::ITextureView* incommingEnvironmentSRV) {

		DG::BufferDesc BuffDesc;
		BuffDesc.Name              = "SH Coeffs Out";
		BuffDesc.Usage             = DG::USAGE_DEFAULT;
		BuffDesc.BindFlags         = DG::BIND_UNIFORM_BUFFER | DG::BIND_UNORDERED_ACCESS;
		BuffDesc.Mode              = DG::BUFFER_MODE_FORMATTED;
		BuffDesc.ElementByteStride = sizeof(DG::float4);
		BuffDesc.uiSizeInBytes     = sizeof(DG::float4) * 9;

		DG::IBuffer* buf = nullptr;
		device->CreateBuffer(BuffDesc, nullptr, &buf);

		DG::BufferViewDesc ViewDesc;
        ViewDesc.ViewType             = DG::BUFFER_VIEW_UNORDERED_ACCESS;
        ViewDesc.Format.ValueType     = DG::VT_FLOAT32;
        ViewDesc.Format.NumComponents = 4;

		DG::IBufferView* view = nullptr;
		buf->CreateView(ViewDesc, &view);

		ComputeIrradiance(context, incommingEnvironmentSRV, view);

		view->Release();

		return buf;
	}

	void LightProbeProcessor::ComputePrefilteredEnvironment(DG::IDeviceContext* context, 
		DG::ITextureView* environmentSRV,
		DG::ITexture* outputCubemap) {

		if (!mPrefilterEnvPipeline) {
			throw std::runtime_error("Initialize has not been called!");
		}
		if (outputCubemap->GetDesc().Format != mConfig.mPrefilteredEnvFormat) {
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

		context->SetPipelineState(mPrefilterEnvPipeline);
		mPrefilterEnvSRB->GetVariableByName(DG::SHADER_TYPE_PIXEL, "g_EnvironmentMap")->Set(environmentSRV);
		context->CommitShaderResources(mPrefilterEnvSRB, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		auto*       pPrefilteredEnvMap    = outputCubemap;
		const auto& PrefilteredEnvMapDesc = pPrefilteredEnvMap->GetDesc();
		for (uint mip = 0; mip < PrefilteredEnvMapDesc.MipLevels; ++mip)
		{
			for (uint face = 0; face < 6; ++face)
			{
				DG::TextureViewDesc RTVDesc(DG::TEXTURE_VIEW_RENDER_TARGET, DG::RESOURCE_DIM_TEX_2D_ARRAY);
				RTVDesc.Name            = "RTV for prefiltered env map cube texture";
				RTVDesc.MostDetailedMip = mip;
				RTVDesc.FirstArraySlice = face;
				RTVDesc.NumArraySlices  = 1;
				DG::RefCntAutoPtr<DG::ITextureView> pRTV;
				pPrefilteredEnvMap->CreateView(RTVDesc, &pRTV);
				DG::ITextureView* ppRTVs[] = {pRTV};
				context->SetRenderTargets(_countof(ppRTVs), ppRTVs, nullptr, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

				{
					DG::MapHelper<PrecomputeEnvMapAttribs> Attribs(context, mTransformConstantBuffer, 
						DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
					Attribs->Rotation   = Matrices[face];
					Attribs->Roughness  = static_cast<float>(mip) / static_cast<float>(PrefilteredEnvMapDesc.MipLevels);
					Attribs->EnvMapDim  = static_cast<float>(PrefilteredEnvMapDesc.Width);
					Attribs->NumSamples = mConfig.mEnvMapSamples;
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

	DG::ITexture* LightProbeProcessor::ComputePrefilteredEnvironment(DG::IRenderDevice* device,
		DG::IDeviceContext* context,
		DG::ITextureView* incommingEnvironmentSRV,
		uint size) {

		DG::TextureDesc desc;
		desc.BindFlags = DG::BIND_RENDER_TARGET | DG::BIND_SHADER_RESOURCE;
		desc.Width = size;
		desc.Height = size;
		desc.MipLevels = 0;
		desc.ArraySize = 6;
		desc.Type = DG::RESOURCE_DIM_TEX_CUBE;
		desc.Usage = DG::USAGE_DEFAULT;
		desc.Name = "Light Probe Prefiltered Environment";
		desc.Format = mConfig.mPrefilteredEnvFormat;

		DG::ITexture* result = nullptr;
		device->CreateTexture(desc, nullptr, &result);

		ComputePrefilteredEnvironment(context, incommingEnvironmentSRV, result);
		return result;
	}

	LightProbe LightProbeProcessor::ComputeLightProbe(
		DG::IRenderDevice* device,
		DG::IDeviceContext* context,
		DG::ITextureView* incommingEnvironmentSRV,
		uint prefilteredEnvironmentSize) {

		Handle<Texture> texture;
		texture.Adopt(new Texture(device, ComputePrefilteredEnvironment(device, context,
			incommingEnvironmentSRV, prefilteredEnvironmentSize)));

		Handle<DG::IBuffer> shBuffer;
		shBuffer.Adopt(ComputeIrradiance(device, context, incommingEnvironmentSRV));

		return LightProbe(std::move(shBuffer), std::move(texture));
	}
}