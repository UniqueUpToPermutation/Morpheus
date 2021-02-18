#include <Engine/Im3d.hpp>
#include <Engine/Resources/ShaderResource.hpp>

#include "MapHelper.hpp"

namespace Morpheus {

	Im3dRenderer::Im3dRenderer(DG::IRenderDevice* device, 
			Im3dRendererFactory* factory,
			uint bufferSize) :
		mGlobals(device),
		mGeometryBuffer(nullptr),
		mPipelineStateVertices(factory->mPipelineStateVertices),
		mPipelineStateLines(factory->mPipelineStateLines),
		mPipelineStateTriangles(factory->mPipelineStateTriangles),
		mBufferSize(bufferSize) {

		mPipelineStateVertices->AddRef();
		mPipelineStateTriangles->AddRef();
		mPipelineStateLines->AddRef();

		DG::BufferDesc CBDesc;
		CBDesc.Name 			= "Im3d Geometry Buffer";
		CBDesc.uiSizeInBytes 	= sizeof(Im3d::VertexData) * bufferSize;
		CBDesc.Usage 			= DG::USAGE_DYNAMIC;
		CBDesc.BindFlags 		= DG::BIND_VERTEX_BUFFER;
		CBDesc.CPUAccessFlags	= DG::CPU_ACCESS_WRITE;

		device->CreateBuffer(CBDesc, nullptr, &mGeometryBuffer);
	}

	void Im3dRendererFactory::Initialize(DG::IRenderDevice* device,
		DG::TEXTURE_FORMAT backbufferColorFormat,
		DG::TEXTURE_FORMAT backbufferDepthFormat,
		uint backbufferMSAASamples) {

		ShaderPreprocessorConfig vsPtConfig;
		vsPtConfig.mDefines["POINTS"] = "1";
		vsPtConfig.mDefines["VERTEX_SHADER"] = "1";

		LoadParams<ShaderResource> vsPointsParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_VERTEX,
			"Im3d Point VS",
			&vsPtConfig);

		ShaderPreprocessorConfig vsLinesConfig;
		vsLinesConfig.mDefines["LINES"] = "1";
		vsLinesConfig.mDefines["VERTEX_SHADER"] = "1";

		LoadParams<ShaderResource> vsLinesParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_VERTEX,
			"Im3d Line VS",
			&vsLinesConfig
		);

		ShaderPreprocessorConfig vsTriConfig;
		vsTriConfig.mDefines["TRIANGLES"] = "1";
		vsTriConfig.mDefines["VERTEX_SHADER"] = "1";

		LoadParams<ShaderResource> vsTrisParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_VERTEX,
			"Im3d Tri VS",
			&vsTriConfig
		);

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

		ShaderPreprocessorConfig psConfig;
		psConfig.mDefines["TRIANGLES"] = "1";
		psConfig.mDefines["PIXEL_SHADER"] = "1";

		LoadParams<ShaderResource> psParams(
			"internal/Im3d.hlsl",
			DG::SHADER_TYPE_PIXEL,
			"Im3d PS",
			&psConfig
		);

		DG::IShader* vsPoints = CompileEmbeddedShader(device, vsPointsParams);
		DG::IShader* vsLines = CompileEmbeddedShader(device, vsLinesParams);
		DG::IShader* vsTriangles = CompileEmbeddedShader(device, vsTrisParams);
		DG::IShader* gsPoints = CompileEmbeddedShader(device, gsPointsParams);
		DG::IShader* gsLines = CompileEmbeddedShader(device, gsLinesParams);
		DG::IShader* ps = CompileEmbeddedShader(device, psParams);

		std::vector<DG::LayoutElement> layoutElements = {
			// Position
			DG::LayoutElement(0, 0, 4, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, DG::LAYOUT_ELEMENT_AUTO_STRIDE, 
				DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			// Color
			DG::LayoutElement(1, 0, 4, DG::VT_UINT8, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, DG::LAYOUT_ELEMENT_AUTO_STRIDE, 
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
		PSOCreateInfo.pPS = ps;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

		device->CreateGraphicsPipelineState(PSOCreateInfo, &mPipelineStateTriangles);


		// Line Pipeline
		GraphicsPipeline.PrimitiveTopology = DG::PRIMITIVE_TOPOLOGY_LINE_LIST;
		PSOCreateInfo.pVS = vsLines;
		PSOCreateInfo.pGS = gsLines;
		PSOCreateInfo.pPS = ps;

		device->CreateGraphicsPipelineState(PSOCreateInfo, &mPipelineStateLines);


		// Point Pipeline
		GraphicsPipeline.PrimitiveTopology = DG::PRIMITIVE_TOPOLOGY_POINT_LIST;
		PSOCreateInfo.pVS = vsPoints;
		PSOCreateInfo.pGS = gsPoints;
		PSOCreateInfo.pPS = ps;

		device->CreateGraphicsPipelineState(PSOCreateInfo, &mPipelineStateVertices);


		vsPoints->Release();
		vsLines->Release();
		vsTriangles->Release();
		gsPoints->Release();
		gsLines->Release();
		ps->Release();
	}

	void Im3dRenderer::Draw(DG::IDeviceContext* deviceContext,
		const Im3dGlobals& globals, 
		Im3d::Context* im3dContext) {
		uint drawListCount = im3dContext->getDrawListCount();

		DG::IPipelineState* currentPipelineState = nullptr;

		mGlobals.Write(deviceContext, globals);

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
					}
					break;
				case Im3d::DrawPrimitive_Lines:
					if (currentPipelineState != mPipelineStateLines) {
						currentPipelineState = mPipelineStateLines;
						deviceContext->SetPipelineState(currentPipelineState);
					}
					break;
				case Im3d::DrawPrimitive_Points:
					if (currentPipelineState != mPipelineStateVertices) {
						currentPipelineState = mPipelineStateVertices;
						deviceContext->SetPipelineState(currentPipelineState);
					}
					break;
			}

			uint currentIndx = 0;
			while (currentIndx < drawList->m_vertexCount) {
				uint vertsToRender = std::min(mBufferSize, drawListCount - currentIndx);

				{
					DG::MapHelper<Im3d::VertexData> vertexMap(deviceContext, mGeometryBuffer, 
						DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
					std::memcpy(vertexMap, &drawList->m_vertexData[currentIndx],
						sizeof(Im3d::VertexData) * vertsToRender);
				}

				DG::DrawAttribs drawAttribs;
				drawAttribs.NumVertices = vertsToRender;
				deviceContext->Draw(drawAttribs);

				currentIndx += vertsToRender;
			}
		}
	}
}