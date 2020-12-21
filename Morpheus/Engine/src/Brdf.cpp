#include <Engine/Brdf.hpp>

#include <Engine/PipelineResource.hpp>
#include <Engine/TextureResource.hpp>
#include <Engine/Engine.hpp>

#include "GraphicsUtilities.h"
#include "MapHelper.hpp"

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

	LightProbeProcessor::LightProbeProcessor(DG::IRenderDevice* device) : 
		mIrradiancePipeline(nullptr), 
		mPrefilterEnvPipeline(nullptr), 
		mSHIrradiancePipeline(nullptr),
		mIrradianceSRB(nullptr),
		mPrefilterEnvSRB(nullptr),
		mSHIrradianceSRB(nullptr),
		mTransformConstantBuffer(nullptr) {

		DG::CreateUniformBuffer(device, sizeof(PrecomputeEnvMapAttribs),
			"Light Probe Processor Constants Buffer", 
			&mTransformConstantBuffer);
	}

	LightProbeProcessor::~LightProbeProcessor() {

		if (mIrradianceSRB)
			mIrradianceSRB->Release();
		if (mPrefilterEnvSRB)
			mPrefilterEnvSRB->Release();
		if (mSHIrradianceSRB)
			mSHIrradianceSRB->Release();

		if (mIrradiancePipeline)
			mIrradiancePipeline->Release();
		if (mPrefilterEnvPipeline)
			mPrefilterEnvPipeline->Release();
		if (mSHIrradiancePipeline)
			mSHIrradiancePipeline->Release();

		mTransformConstantBuffer->Release();
	}

	void LightProbeProcessor::Initialize(
		ResourceManager* resourceManager,
		DG::TEXTURE_FORMAT irradianceFormat,
		DG::TEXTURE_FORMAT prefilterEnvFormat,
		const LightProbeProcessorConfig& config) {

		mIrradianceFormat = irradianceFormat;
		mPrefilteredEnvFormat = prefilterEnvFormat;

		mEnvironmentMapSamples = config.mEnvMapSamples;
		
		auto pipelineCache = resourceManager->GetCache<PipelineResource>();
		auto loader = pipelineCache->GetLoader();
		auto device = resourceManager->GetParent()->GetDevice();

		ShaderPreprocessorConfig irradianceConfig;
		ShaderPreprocessorConfig prefilterEnvConfig;
		ShaderPreprocessorConfig irradianceSHConfig;

		irradianceConfig.mDefines["NUM_PHI_SAMPLES"] = std::to_string(config.mIrradianceSamplesPhi);
		irradianceConfig.mDefines["NUM_THETA_SAMPLES"] = std::to_string(config.mIrradianceSamplesTheta);
		prefilterEnvConfig.mDefines["OPTIMIZE_SAMPLES"] = std::to_string((int)config.bEnvMapOptimizeSamples);
		irradianceSHConfig.mDefines["SAMPLE_COUNT"] = std::to_string(config.mIrradianceSHSamples);

		auto cubemapFaceVS = loader->LoadShader(DG::SHADER_TYPE_VERTEX,
			"internal/CubemapFace.vsh",
			"Cubemap Face Vertex Shader",
			"main",
			nullptr);

		auto irradiancePS = loader->LoadShader(DG::SHADER_TYPE_PIXEL,
			"internal/ComputeIrradiance.psh",
			"Compute Irradiance Pixel Shader",
			"main",
			&irradianceConfig);

		auto environmentPS = loader->LoadShader(DG::SHADER_TYPE_PIXEL,
			"internal/PrefilterEnvironment.psh",
			"Compute Environment Pixel Shader",
			"main",
			&prefilterEnvConfig);

		auto irradianceSHCS = loader->LoadShader(DG::SHADER_TYPE_COMPUTE,
			"internal/ComputeIrradianceSH.csh",
			"Compute Irradiance SH Compute Shader",
			"main",
			&irradianceSHConfig);

		DG::SamplerDesc SamLinearClampDesc
		{
			DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, DG::FILTER_TYPE_LINEAR, 
			DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
		};

		{
			// Create Irradiance Pipeline
			DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
			DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
			DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

			PSODesc.Name         = "Precompute irradiance cube PSO";
			PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

			GraphicsPipeline.NumRenderTargets             = 1;
			GraphicsPipeline.RTVFormats[0]                = irradianceFormat;
			GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_NONE;
			GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

			PSOCreateInfo.pVS = cubemapFaceVS;
			PSOCreateInfo.pPS = irradiancePS;

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

			device->CreateGraphicsPipelineState(PSOCreateInfo, &mIrradiancePipeline);
			mIrradiancePipeline->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "mTransform")->Set(mTransformConstantBuffer);
			mIrradiancePipeline->CreateShaderResourceBinding(&mIrradianceSRB, true);
		}

		{
			// Create Environment Pipeline
			DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
			DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
			DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

			PSODesc.Name         = "Prefilter environment map PSO";
			PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

			GraphicsPipeline.NumRenderTargets             = 1;
			GraphicsPipeline.RTVFormats[0]                = prefilterEnvFormat;
			GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_NONE;
			GraphicsPipeline.DepthStencilDesc.DepthEnable = false;

			PSOCreateInfo.pVS = cubemapFaceVS;
			PSOCreateInfo.pPS = environmentPS;

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

			device->CreateGraphicsPipelineState(PSOCreateInfo, &mPrefilterEnvPipeline);
			mPrefilterEnvPipeline->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "mTransform")->Set(mTransformConstantBuffer);
			mPrefilterEnvPipeline->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "FilterAttribs")->Set(mTransformConstantBuffer);
			mPrefilterEnvPipeline->CreateShaderResourceBinding(&mPrefilterEnvSRB, true);
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
			PSOCreateInfo.pCS = irradianceSHCS;

			device->CreateComputePipelineState(PSOCreateInfo, &mSHIrradiancePipeline);
			mSHIrradiancePipeline->CreateShaderResourceBinding(&mSHIrradianceSRB, true);
		}

		cubemapFaceVS->Release();
		irradiancePS->Release();
		environmentPS->Release();
		irradianceSHCS->Release();
	}

	void LightProbeProcessor::ComputeIrradiance(DG::IDeviceContext* context, 
		DG::ITextureView* environmentSRV,
		DG::ITexture* outputCubemap) {

		if (!mIrradiancePipeline) {
			throw std::runtime_error("Initialize has not been called!");
		}
		if (outputCubemap->GetDesc().Format != mIrradianceFormat) {
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

		context->SetPipelineState(mIrradiancePipeline);
		mIrradianceSRB->GetVariableByName(DG::SHADER_TYPE_PIXEL, "g_EnvironmentMap")->Set(environmentSRV);
		context->CommitShaderResources(mIrradianceSRB, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		auto*       pIrradianceCube    = outputCubemap;
		const auto& IrradianceCubeDesc = pIrradianceCube->GetDesc();
		for (uint mip = 0; mip < IrradianceCubeDesc.MipLevels; ++mip)
		{
			for (uint face = 0; face < 6; ++face)
			{
				DG::TextureViewDesc RTVDesc(DG::TEXTURE_VIEW_RENDER_TARGET, DG::RESOURCE_DIM_TEX_2D_ARRAY);
				RTVDesc.Name            = "RTV for irradiance cube texture";
				RTVDesc.MostDetailedMip = mip;
				RTVDesc.FirstArraySlice = face;
				RTVDesc.NumArraySlices  = 1;
				DG::RefCntAutoPtr<DG::ITextureView> pRTV;
				pIrradianceCube->CreateView(RTVDesc, &pRTV);
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

	void LightProbeProcessor::ComputeIrradianceSH(
		DG::IDeviceContext* context,
		DG::ITextureView* incommingEnvironmentSRV,
		DG::IBufferView* outputBufferView) {

		if (!mSHIrradiancePipeline) {
			throw std::runtime_error("Initialize has not been called!");
		}

		mSHIrradianceSRB->GetVariableByName(DG::SHADER_TYPE_COMPUTE, "mEnvironmentMap")->Set(incommingEnvironmentSRV);
		mSHIrradianceSRB->GetVariableByName(DG::SHADER_TYPE_COMPUTE, "mCoeffsOut")->Set(outputBufferView);
		context->CommitShaderResources(mSHIrradianceSRB, DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
		
		DG::DispatchComputeAttribs attribs;		
		context->DispatchCompute(attribs);
	}

	DG::IBuffer* LightProbeProcessor::ComputeIrradianceSH(
		DG::IRenderDevice* device,
		DG::IDeviceContext* context,
		DG::ITextureView* incommingEnvironmentSRV) {

		DG::BufferDesc BuffDesc;
		BuffDesc.Name              = "SH Coeffs Out";
		BuffDesc.Usage             = DG::USAGE_DEFAULT;
		BuffDesc.BindFlags         = DG::BIND_SHADER_RESOURCE | DG::BIND_UNORDERED_ACCESS;
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

		ComputeIrradianceSH(context, incommingEnvironmentSRV, view);

		view->Release();

		return buf;
	}

	void LightProbeProcessor::ComputePrefilteredEnvironment(DG::IDeviceContext* context, 
		DG::ITextureView* environmentSRV,
		DG::ITexture* outputCubemap) {

		if (!mPrefilterEnvPipeline) {
			throw std::runtime_error("Initialize has not been called!");
		}
		if (outputCubemap->GetDesc().Format != mPrefilteredEnvFormat) {
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
					Attribs->NumSamples = mEnvironmentMapSamples;
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

	DG::ITexture* LightProbeProcessor::ComputeIrradiance(DG::IRenderDevice* device,
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
		desc.Name = "Light Probe Irradiance";
		desc.Format = mIrradianceFormat;

		DG::ITexture* result = nullptr;
		device->CreateTexture(desc, nullptr, &result);

		ComputeIrradiance(context, incommingEnvironmentSRV, result);
		return result;
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
		desc.Format = mPrefilteredEnvFormat;

		DG::ITexture* result = nullptr;
		device->CreateTexture(desc, nullptr, &result);

		ComputePrefilteredEnvironment(context, incommingEnvironmentSRV, result);
		return result;
	}

	LightProbe LightProbeProcessor::ComputeLightProbe(
		DG::IRenderDevice* device,
		DG::IDeviceContext* context,
		ResourceCache<TextureResource>* textureCache,
		DG::ITextureView* incommingEnvironmentSRV,
		uint prefilteredEnvironmentSize,
		uint irradianceSize) {

		DG::ITexture* irradiance = ComputeIrradiance(device, context, incommingEnvironmentSRV, irradianceSize);
		DG::ITexture* preEnv = ComputePrefilteredEnvironment(device, context, incommingEnvironmentSRV, prefilteredEnvironmentSize);

		TextureResource* irradianceResource = textureCache->MakeResource(irradiance);
		TextureResource* preEnvResource = textureCache->MakeResource(preEnv);

		LightProbe probe(irradianceResource, preEnvResource);
		
		irradianceResource->Release();
		preEnvResource->Release();
	
		return probe;
	}
}