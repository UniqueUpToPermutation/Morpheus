#include <Engine/Im3d.hpp>
#include <Engine/Resources/ShaderResource.hpp>
#include <Engine/Camera.hpp>

#include "MapHelper.hpp"

namespace Morpheus {

	void Im3dGlobalsBuffer::Write(DG::IDeviceContext* context,
			EntityNode camera,
			Engine* engine) {

		auto& scDesc = engine->GetSwapChain()->GetDesc();

		DG::float3 eye;
		DG::float3 lookAt;
		DG::float4x4 view;
		DG::float4x4 proj;
		DG::float4x4 viewProj;

		Camera::ComputeTransformations(camera, engine,
			&eye, &lookAt, &view, &proj, &viewProj);

		DynamicGlobalsBuffer<Im3dGlobals>::Write(context,
			Im3dGlobals{viewProj, DG::float2(scDesc.Width, scDesc.Height)});
	}

	void Im3dGlobalsBuffer::Write(DG::IDeviceContext* context, 
		const DG::float4x4& viewProjection,
		const DG::float2& screenSize) {
		DynamicGlobalsBuffer<Im3dGlobals>::Write(context,
			Im3dGlobals{viewProjection, screenSize});
	}

	Im3dRenderer::Im3dRenderer(DG::IRenderDevice* device, 
			Im3dRendererFactory* factory,
			uint bufferSize) :
		mGeometryBuffer(nullptr),
		mPipelineStateVertices(factory->mPipelineStateVertices),
		mPipelineStateLines(factory->mPipelineStateLines),
		mPipelineStateTriangles(factory->mPipelineStateTriangles),
		mVertexSRB(factory->mVertexSRB),
		mLinesSRB(factory->mLinesSRB),
		mTriangleSRB(factory->mTriangleSRB),
		mBufferSize(bufferSize) {

		mPipelineStateVertices->AddRef();
		mPipelineStateTriangles->AddRef();
		mPipelineStateLines->AddRef();

		mVertexSRB->AddRef();
		mLinesSRB->AddRef();
		mTriangleSRB->AddRef();

		DG::BufferDesc CBDesc;
		CBDesc.Name 			= "Im3d Geometry Buffer";
		CBDesc.uiSizeInBytes 	= sizeof(Im3d::VertexData) * bufferSize;
		CBDesc.Usage 			= DG::USAGE_DYNAMIC;
		CBDesc.BindFlags 		= DG::BIND_VERTEX_BUFFER;
		CBDesc.CPUAccessFlags	= DG::CPU_ACCESS_WRITE;

		device->CreateBuffer(CBDesc, nullptr, &mGeometryBuffer);
	}

	void Im3dRendererFactory::Initialize(DG::IRenderDevice* device,
		Im3dGlobalsBuffer* globals,
		DG::TEXTURE_FORMAT backbufferColorFormat,
		DG::TEXTURE_FORMAT backbufferDepthFormat,
		uint backbufferMSAASamples) {

		ShaderPreprocessorConfig vsTrianglesConfig;
		vsTrianglesConfig.mDefines["TRIANGLES"] = "1";
		vsTrianglesConfig.mDefines["VERTEX_SHADER"] = "1";

		LoadParams<ShaderResource> vsTrianglesParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_VERTEX,
			"Im3d Triangle VS",
			&vsTrianglesConfig);

		ShaderPreprocessorConfig vsOtherConfig;
		vsOtherConfig.mDefines["POINTS"] = "1";
		vsOtherConfig.mDefines["VERTEX_SHADER"] = "1";

		LoadParams<ShaderResource> vsOtherParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_VERTEX,
			"Im3d Other VS",
			&vsOtherConfig);

		ShaderPreprocessorConfig gsPtConfig;
		gsPtConfig.mDefines["POINTS"] = "1";
		gsPtConfig.mDefines["GEOMETRY_SHADER"] = "1";

		LoadParams<ShaderResource> gsPointsParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_GEOMETRY,
			"Im3d Point GS",
			&gsPtConfig
		);

		ShaderPreprocessorConfig gsLineConfig;
		gsLineConfig.mDefines["LINES"] = "1";
		gsLineConfig.mDefines["GEOMETRY_SHADER"] = "1";

		LoadParams<ShaderResource> gsLinesParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_GEOMETRY,
			"Im3d Line GS",
			&gsLineConfig
		);

		ShaderPreprocessorConfig psTriangleConfig;
		psTriangleConfig.mDefines["TRIANGLES"] = "1";
		psTriangleConfig.mDefines["PIXEL_SHADER"] = "1";

		LoadParams<ShaderResource> psTriangleParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_PIXEL,
			"Im3d Triangle PS",
			&psTriangleConfig
		);

		ShaderPreprocessorConfig psLinesConfig;
		psLinesConfig.mDefines["LINES"] = "1";
		psLinesConfig.mDefines["PIXEL_SHADER"] = "1";

		LoadParams<ShaderResource> psLinesParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_PIXEL,
			"Im3d Lines PS",
			&psLinesConfig
		);

		ShaderPreprocessorConfig psPointConfig;
		psPointConfig.mDefines["POINTS"] = "1";
		psPointConfig.mDefines["PIXEL_SHADER"] = "1";

		LoadParams<ShaderResource> psPointParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_PIXEL,
			"Im3d Point PS",
			&psPointConfig
		);

		DG::IShader* vsTriangles = CompileEmbeddedShader(device, vsTrianglesParams);
		DG::IShader* gsPoints = CompileEmbeddedShader(device, gsPointsParams);
		DG::IShader* gsLines = CompileEmbeddedShader(device, gsLinesParams);
		DG::IShader* psPoints = CompileEmbeddedShader(device, psPointParams);
		DG::IShader* psLines = CompileEmbeddedShader(device, psLinesParams);
		DG::IShader* psTriangles = CompileEmbeddedShader(device, psTriangleParams);
		DG::IShader* vsOther = CompileEmbeddedShader(device, vsOtherParams);

		size_t stride = sizeof(Im3d::VertexData);
		size_t position_offset = offsetof(Im3d::VertexData, m_positionSize);
		size_t color_offset = offsetof(Im3d::VertexData, m_color);

		std::vector<DG::LayoutElement> layoutElements = {
			// Position
			DG::LayoutElement(0, 0, 4, DG::VT_FLOAT32, false, 
				position_offset, stride, 
				DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			// Color
			DG::LayoutElement(1, 0, 4, DG::VT_UINT8, true, 
				color_offset, stride, 
				DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
		};

		// Create 
		DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
		DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
		DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name         = "Im3d Triangle Pipeline";
		PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets             = 1;
		GraphicsPipeline.RTVFormats[0]                = backbufferColorFormat;
		GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		GraphicsPipeline.DSVFormat 					  = backbufferDepthFormat;

		DG::RenderTargetBlendDesc blendState;
		blendState.BlendEnable = true;
		blendState.BlendOp = DG::BLEND_OPERATION_ADD;
		blendState.BlendOpAlpha = DG::BLEND_OPERATION_ADD;
		blendState.DestBlend = DG::BLEND_FACTOR_INV_SRC_ALPHA;
		blendState.SrcBlend = DG::BLEND_FACTOR_SRC_ALPHA;
		blendState.DestBlendAlpha = DG::BLEND_FACTOR_ONE;
		blendState.SrcBlendAlpha = DG::BLEND_FACTOR_ONE;

		GraphicsPipeline.BlendDesc.RenderTargets[0] = blendState;

		// Number of MSAA samples
		GraphicsPipeline.SmplDesc.Count = backbufferMSAASamples;

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = vsTriangles;
		PSOCreateInfo.pPS = psTriangles;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

		device->CreateGraphicsPipelineState(PSOCreateInfo, &mPipelineStateTriangles);
		mPipelineStateTriangles->GetStaticVariableByName(
			DG::SHADER_TYPE_VERTEX, "cbContextData")->Set(globals->Get());

		// Line Pipeline
		GraphicsPipeline.PrimitiveTopology = DG::PRIMITIVE_TOPOLOGY_LINE_LIST;
		PSOCreateInfo.pVS = vsOther;
		PSOCreateInfo.pGS = gsLines;
		PSOCreateInfo.pPS = psLines;

		device->CreateGraphicsPipelineState(PSOCreateInfo, &mPipelineStateLines);
		mPipelineStateLines->GetStaticVariableByName(
			DG::SHADER_TYPE_VERTEX, "cbContextData")->Set(globals->Get());
		mPipelineStateLines->GetStaticVariableByName(
			DG::SHADER_TYPE_GEOMETRY, "cbContextData")->Set(globals->Get());				

		// Point Pipeline
		GraphicsPipeline.PrimitiveTopology = DG::PRIMITIVE_TOPOLOGY_POINT_LIST;
		PSOCreateInfo.pVS = vsOther;
		PSOCreateInfo.pGS = gsPoints;
		PSOCreateInfo.pPS = psPoints;

		device->CreateGraphicsPipelineState(PSOCreateInfo, &mPipelineStateVertices);
		mPipelineStateVertices->GetStaticVariableByName(
			DG::SHADER_TYPE_VERTEX, "cbContextData")->Set(globals->Get());
		mPipelineStateVertices->GetStaticVariableByName(
			DG::SHADER_TYPE_GEOMETRY, "cbContextData")->Set(globals->Get());				

		mPipelineStateVertices->CreateShaderResourceBinding(&mVertexSRB, true);
		mPipelineStateLines->CreateShaderResourceBinding(&mLinesSRB, true);
		mPipelineStateTriangles->CreateShaderResourceBinding(&mTriangleSRB, true);

		vsTriangles->Release();
		vsOther->Release();
		gsPoints->Release();
		gsLines->Release();
		psPoints->Release();
		psLines->Release();
		psTriangles->Release();
	}

	void Im3dRenderer::Draw(DG::IDeviceContext* deviceContext,
		Im3d::Context* im3dContext) {
		uint drawListCount = im3dContext->getDrawListCount();

		DG::IPipelineState* currentPipelineState = nullptr;

		uint offsets[] = {0};
		deviceContext->SetVertexBuffers(0, 1, 
			&mGeometryBuffer, offsets, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
			DG::SET_VERTEX_BUFFERS_FLAG_RESET);

		for (uint i = 0; i < drawListCount; ++i) {
			auto drawList = &im3dContext->getDrawLists()[i];

			switch (drawList->m_primType) {
				case Im3d::DrawPrimitiveType::DrawPrimitive_Triangles:
					if (currentPipelineState != mPipelineStateTriangles) {
						currentPipelineState = mPipelineStateTriangles;
						deviceContext->SetPipelineState(currentPipelineState);
						deviceContext->CommitShaderResources(mTriangleSRB, 
							DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
					}
					break;
				case Im3d::DrawPrimitive_Lines:
					if (currentPipelineState != mPipelineStateLines) {
						currentPipelineState = mPipelineStateLines;
						deviceContext->SetPipelineState(currentPipelineState);
						deviceContext->CommitShaderResources(mLinesSRB, 
							DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
					}
					break;
				case Im3d::DrawPrimitive_Points:
					if (currentPipelineState != mPipelineStateVertices) {
						currentPipelineState = mPipelineStateVertices;
						deviceContext->SetPipelineState(currentPipelineState);
						deviceContext->CommitShaderResources(mVertexSRB, 
							DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
					}
					break;
			}

			uint currentIndx = 0;
			while (currentIndx < drawList->m_vertexCount) {
				uint vertsToRender = std::min(mBufferSize, drawList->m_vertexCount - currentIndx);

				{
					DG::MapHelper<Im3d::VertexData> vertexMap(deviceContext, mGeometryBuffer, 
						DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
					std::memcpy(vertexMap, &drawList->m_vertexData[currentIndx],
						sizeof(Im3d::VertexData) * vertsToRender);
				}

				DG::DrawAttribs drawAttribs;
				drawAttribs.NumVertices = vertsToRender;
				drawAttribs.Flags = DG::DRAW_FLAG_VERIFY_ALL;

				deviceContext->Draw(drawAttribs);

				currentIndx += vertsToRender;
			}
		}
	}
}