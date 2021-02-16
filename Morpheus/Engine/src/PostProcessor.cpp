#include <Engine/PostProcessor.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Engine.hpp>
#include <Engine/Resources/ShaderResource.hpp>

#include "MapHelper.hpp"
#include "GraphicsUtilities.h"

namespace Morpheus {

	PostProcessor::PostProcessor(DG::IRenderDevice* device) :
		mPipeline(nullptr),
		mShaderResources(nullptr),
		mParameterBuffer(nullptr) {

		DG::CreateUniformBuffer(device, sizeof(PostProcessorParams),
			"Post Processor Uniform Buffer", 
			&mParameterBuffer);
	}

	PostProcessor::~PostProcessor() {
		mParameterBuffer->Release();

		if (mPipeline)
			mPipeline->Release();
		if (mShaderResources)
			mShaderResources->Release();
	}

	void PostProcessor::SetAttributes(DG::IDeviceContext* context, const PostProcessorParams& params) {
		DG::MapHelper<PostProcessorParams> helper(context, 
			mParameterBuffer, 
			DG::MAP_WRITE,
			DG::MAP_FLAG_DISCARD);

		*helper = params;
	}

	void PostProcessor::Initialize(ResourceManager* resourceManager,
		DG::TEXTURE_FORMAT renderTargetColorFormat,
		DG::TEXTURE_FORMAT depthStencilFormat) {

		auto device = resourceManager->GetParent()->GetDevice();

		LoadParams<ShaderResource> vsParams("internal/FullscreenTriangle.vsh",
			DG::SHADER_TYPE_VERTEX,
			"Post Processor VS",
			nullptr,
			"main");

		LoadParams<ShaderResource> psParams("internal/PostProcessor.psh",
			DG::SHADER_TYPE_PIXEL,
			"Post Processor PS",
			nullptr,
			"main");

		auto fullscreenTriangleVSResource = resourceManager->Load<ShaderResource>(vsParams);
		auto postProcessorPSResource = resourceManager->Load<ShaderResource>(psParams);

		auto fullscreenTriangleVS = fullscreenTriangleVSResource->GetShader();
		auto postProcessorPS = postProcessorPSResource->GetShader();

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

			PSODesc.Name         = "Post Processor Pipeline";
			PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

			GraphicsPipeline.NumRenderTargets             = 1;
			GraphicsPipeline.RTVFormats[0]                = renderTargetColorFormat;
			GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_NONE;
			GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
			GraphicsPipeline.DSVFormat 					  = depthStencilFormat;

			PSOCreateInfo.pVS = fullscreenTriangleVS;
			PSOCreateInfo.pPS = postProcessorPS;

			PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
			// clang-format off
			DG::ShaderResourceVariableDesc Vars[] = 
			{
				{DG::SHADER_TYPE_PIXEL, "mInputFramebuffer", DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
			};
			// clang-format on
			PSODesc.ResourceLayout.NumVariables = _countof(Vars);
			PSODesc.ResourceLayout.Variables    = Vars;

			// clang-format off
			DG::ImmutableSamplerDesc ImtblSamplers[] =
			{
				{DG::SHADER_TYPE_PIXEL, "mInputFramebuffer_sampler", SamLinearClampDesc}
			};
			// clang-format on
			PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
			PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;

			device->CreateGraphicsPipelineState(PSOCreateInfo, &mPipeline);

			auto ppAttribsVar = mPipeline->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "mAttribs");
			if (ppAttribsVar)
				ppAttribsVar->Set(mParameterBuffer);
				
			mPipeline->CreateShaderResourceBinding(&mShaderResources, true);
		}

		postProcessorPSResource->Release();
		fullscreenTriangleVSResource->Release();

		mInputFramebuffer = mShaderResources->GetVariableByName(DG::SHADER_TYPE_PIXEL, 
			"mInputFramebuffer");
	}

	void PostProcessor::Draw(DG::IDeviceContext* context, DG::ITextureView* inputShaderResource) {

		mInputFramebuffer->Set(inputShaderResource);

		context->SetPipelineState(mPipeline);
		context->CommitShaderResources(mShaderResources, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		DG::DrawAttribs attribs;

		attribs.NumVertices = 3;
		attribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;
		context->Draw(attribs);
	}
}