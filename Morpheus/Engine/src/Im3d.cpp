#include <Engine/Im3d.hpp>
#include <Engine/Resources/Shader.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Graphics.hpp>

#include "MapHelper.hpp"

namespace Morpheus {

	void Im3dGlobalsBuffer::Write(DG::IDeviceContext* context,
			RealtimeGraphics& graphics,
			entt::entity camera,
			entt::registry* registry) {

		auto& scDesc = graphics.SwapChain()->GetDesc();

		DG::float3 eye;
		DG::float3 lookAt;
		DG::float4x4 view;
		DG::float4x4 proj;
		DG::float4x4 viewProj;

		auto& cameraComponent = registry->get<Camera>(camera);
		cameraComponent.ComputeTransformations(camera, registry, graphics.SwapChain(), graphics.IsGL(), 
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

	void Im3dGlobalsBuffer::WriteWithoutTransformCache(DG::IDeviceContext* context,
		RealtimeGraphics& graphics,
		const Camera& camera) {
		auto& scDesc = graphics.SwapChain()->GetDesc();

		DG::float4x4 view = camera.GetView();
		DG::float4x4 proj = camera.GetProjection(graphics.SwapChain(), graphics.IsGL());
		DG::float4x4 viewProj = view * proj;

		DG::float4x4 translation = view * proj;

		DynamicGlobalsBuffer<Im3dGlobals>::Write(context,
			Im3dGlobals{translation, DG::float2(scDesc.Width, scDesc.Height)});
	}

	Future<Im3dShaders> Im3dShaders::LoadDefault(DG::IRenderDevice* device, 
		IVirtualFileSystem* system) {

		Promise<Im3dShaders> promise;
		Future<Im3dShaders> future(promise);

		struct {
			Future<Handle<DG::IShader>> mTrianglesVS;
			Future<Handle<DG::IShader>> mOtherVS;
			Future<Handle<DG::IShader>> mPointsGS;
			Future<Handle<DG::IShader>> mLinesGS;
			Future<Handle<DG::IShader>> mTrianglesPS;
			Future<Handle<DG::IShader>> mLinesPS;
			Future<Handle<DG::IShader>> mPointsPS;
		} data;

		ShaderPreprocessorConfig vsTrianglesConfig;
		vsTrianglesConfig.mDefines["TRIANGLES"] = "1";
		vsTrianglesConfig.mDefines["VERTEX_SHADER"] = "1";

		LoadParams<RawShader> vsTrianglesParams(
			"Im3d/Im3d.hlsl",
			DG::SHADER_TYPE_VERTEX,
			"Im3d Triangle VS",
			vsTrianglesConfig);

		ShaderPreprocessorConfig vsOtherConfig;
		vsOtherConfig.mDefines["POINTS"] = "1";
		vsOtherConfig.mDefines["VERTEX_SHADER"] = "1";

		LoadParams<RawShader> vsOtherParams(
			"Im3d/Im3d.hlsl",
			DG::SHADER_TYPE_VERTEX,
			"Im3d Other VS",
			vsOtherConfig);

		ShaderPreprocessorConfig gsPtConfig;
		gsPtConfig.mDefines["POINTS"] = "1";
		gsPtConfig.mDefines["GEOMETRY_SHADER"] = "1";

		LoadParams<RawShader> gsPointsParams(
			"Im3d/Im3d.hlsl",
			DG::SHADER_TYPE_GEOMETRY,
			"Im3d Point GS",
			gsPtConfig
		);

		ShaderPreprocessorConfig gsLineConfig;
		gsLineConfig.mDefines["LINES"] = "1";
		gsLineConfig.mDefines["GEOMETRY_SHADER"] = "1";

		LoadParams<RawShader> gsLinesParams(
			"Im3d/Im3d.hlsl",
			DG::SHADER_TYPE_GEOMETRY,
			"Im3d Line GS",
			gsLineConfig
		);

		ShaderPreprocessorConfig psTriangleConfig;
		psTriangleConfig.mDefines["TRIANGLES"] = "1";
		psTriangleConfig.mDefines["PIXEL_SHADER"] = "1";

		LoadParams<RawShader> psTriangleParams(
			"Im3d/Im3d.hlsl",
			DG::SHADER_TYPE_PIXEL,
			"Im3d Triangle PS",
			psTriangleConfig
		);

		ShaderPreprocessorConfig psLinesConfig;
		psLinesConfig.mDefines["LINES"] = "1";
		psLinesConfig.mDefines["PIXEL_SHADER"] = "1";

		LoadParams<RawShader> psLinesParams(
			"Im3d/Im3d.hlsl",
			DG::SHADER_TYPE_PIXEL,
			"Im3d Lines PS",
			psLinesConfig
		);

		ShaderPreprocessorConfig psPointConfig;
		psPointConfig.mDefines["POINTS"] = "1";
		psPointConfig.mDefines["PIXEL_SHADER"] = "1";

		LoadParams<RawShader> psPointParams(
			"Im3d/Im3d.hlsl",
			DG::SHADER_TYPE_PIXEL,
			"Im3d Point PS",
			psPointConfig
		);

		data.mTrianglesVS = LoadShaderHandle(device, vsTrianglesParams, system);
		data.mOtherVS = LoadShaderHandle(device, vsOtherParams, system);
		data.mPointsGS = LoadShaderHandle(device, gsPointsParams, system);
		data.mLinesGS = LoadShaderHandle(device, gsLinesParams, system);
		data.mTrianglesPS = LoadShaderHandle(device, psTriangleParams, system);
		data.mLinesPS = LoadShaderHandle(device, psLinesParams, system);
		data.mPointsPS = LoadShaderHandle(device, psPointParams, system);

		FunctionPrototype<
			Future<Handle<DG::IShader>>,
			Future<Handle<DG::IShader>>,
			Future<Handle<DG::IShader>>,
			Future<Handle<DG::IShader>>,
			Future<Handle<DG::IShader>>,
			Future<Handle<DG::IShader>>,
			Future<Handle<DG::IShader>>,
			Promise<Im3dShaders>> prototype([](
				const TaskParams& e,
				Future<Handle<DG::IShader>> triangleVS,
				Future<Handle<DG::IShader>> otherVS,
				Future<Handle<DG::IShader>> pointsGS,
				Future<Handle<DG::IShader>> linesGS,
				Future<Handle<DG::IShader>> trianglesPS,
				Future<Handle<DG::IShader>> linesPS,
				Future<Handle<DG::IShader>> pointsPS,
				Promise<Im3dShaders> output
			) {
			Im3dShaders shaders;
			shaders.mTrianglesVS = triangleVS.Get();
			shaders.mOtherVS = otherVS.Get();
			shaders.mPointsGS = pointsGS.Get();
			shaders.mLinesGS = linesGS.Get();
			shaders.mTrianglesPS = trianglesPS.Get();
			shaders.mLinesPS = linesPS.Get();
			shaders.mPointsPS = pointsPS.Get();
			output = std::move(shaders);
		});

		Promise<Im3dShaders> output;

		prototype(data.mTrianglesVS,
			data.mOtherVS,
			data.mPointsGS,
			data.mLinesGS,
			data.mTrianglesPS,
			data.mLinesPS,
			data.mPointsPS,
			output).SetName("Create Im3dShaders Struct");

		return output;
	}

	Im3dPipeline::Im3dPipeline(DG::IRenderDevice* device,
		Im3dGlobalsBuffer* globals,
		DG::TEXTURE_FORMAT backbufferColorFormat,
		DG::TEXTURE_FORMAT backbufferDepthFormat,
		uint samples,
		const Im3dShaders& shaders) : mShaders(shaders) {

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
		GraphicsPipeline.SmplDesc.Count = samples;

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

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = shaders.mTrianglesVS;
		PSOCreateInfo.pGS = nullptr;
		PSOCreateInfo.pPS = shaders.mTrianglesPS;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

		DG::IPipelineState* pipelineStateTris = nullptr;
		device->CreateGraphicsPipelineState(PSOCreateInfo, &pipelineStateTris);
		mPipelineStateTriangles.Adopt(pipelineStateTris);

		auto contextData = mPipelineStateTriangles->GetStaticVariableByName(
			DG::SHADER_TYPE_VERTEX, "cbContextData");
		if (contextData) contextData->Set(globals->Get());

		// Line Pipeline
		GraphicsPipeline.PrimitiveTopology = DG::PRIMITIVE_TOPOLOGY_LINE_LIST;
		PSOCreateInfo.pVS = shaders.mOtherVS;
		PSOCreateInfo.pGS = shaders.mLinesGS;
		PSOCreateInfo.pPS = shaders.mLinesPS;
		PSODesc.Name = "Im3d Lines Pipeline";

		DG::IPipelineState* pipelineStateLines = nullptr;
		device->CreateGraphicsPipelineState(PSOCreateInfo, &pipelineStateLines);
		mPipelineStateLines.Adopt(pipelineStateLines);

		contextData = mPipelineStateLines->GetStaticVariableByName(
			DG::SHADER_TYPE_VERTEX, "cbContextData");
		if (contextData) contextData->Set(globals->Get());

		contextData = mPipelineStateLines->GetStaticVariableByName(
			DG::SHADER_TYPE_GEOMETRY, "cbContextData");
		if (contextData) contextData->Set(globals->Get());				

		// Point Pipeline
		GraphicsPipeline.PrimitiveTopology = DG::PRIMITIVE_TOPOLOGY_POINT_LIST;
		PSOCreateInfo.pVS = shaders.mOtherVS;
		PSOCreateInfo.pGS = shaders.mPointsGS;
		PSOCreateInfo.pPS = shaders.mPointsPS;
		PSODesc.Name = "Im3d Points Pipeline";

		auto shaderType = shaders.mPointsGS->GetDesc().ShaderType;

		DG::IPipelineState* pipelineStateVertices = nullptr;
		device->CreateGraphicsPipelineState(PSOCreateInfo, &pipelineStateVertices);
		mPipelineStateVertices.Adopt(pipelineStateVertices);

		contextData = mPipelineStateVertices->GetStaticVariableByName(
			DG::SHADER_TYPE_VERTEX, "cbContextData");
		if (contextData) contextData->Set(globals->Get());

		contextData = mPipelineStateVertices->GetStaticVariableByName(
			DG::SHADER_TYPE_GEOMETRY, "cbContextData");
		if (contextData) contextData->Set(globals->Get());

		mShaders = shaders;

		DG::IShaderResourceBinding* vertBinding = nullptr;
		mPipelineStateVertices->CreateShaderResourceBinding(&vertBinding, true);
		mVertexSRB.Adopt(vertBinding);

		DG::IShaderResourceBinding* lineBinding = nullptr;
		mPipelineStateLines->CreateShaderResourceBinding(&lineBinding, true);
		mLinesSRB.Adopt(lineBinding);

		DG::IShaderResourceBinding* triangleBinding = nullptr;
		mPipelineStateTriangles->CreateShaderResourceBinding(&triangleBinding, true);
		mTriangleSRB.Adopt(triangleBinding);
	}

	Im3dRenderer::Im3dRenderer(DG::IRenderDevice* device, 
			uint bufferSize) :
		mGeometryBuffer(nullptr),
		mBufferSize(bufferSize) {

		DG::BufferDesc CBDesc;
		CBDesc.Name 			= "Im3d Geometry Buffer";
		CBDesc.uiSizeInBytes 	= sizeof(Im3d::VertexData) * bufferSize;
		CBDesc.Usage 			= DG::USAGE_DYNAMIC;
		CBDesc.BindFlags 		= DG::BIND_VERTEX_BUFFER;
		CBDesc.CPUAccessFlags	= DG::CPU_ACCESS_WRITE;

		DG::IBuffer* geoBuf = nullptr;
		device->CreateBuffer(CBDesc, nullptr, &geoBuf);
		mGeometryBuffer.Adopt(geoBuf);
	}

	void Im3dRenderer::Draw(DG::IDeviceContext* deviceContext,
		const Im3dPipeline& pipeline,
		Im3d::Context* im3dContext) {
		uint drawListCount = im3dContext->getDrawListCount();

		DG::IPipelineState* currentPipelineState = nullptr;

		uint offsets[] = {0};
		DG::IBuffer* vBuffers[] = { mGeometryBuffer.Ptr()};

		deviceContext->SetVertexBuffers(0, 1, 
			vBuffers, offsets, 
			DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
			DG::SET_VERTEX_BUFFERS_FLAG_RESET);

		for (uint i = 0; i < drawListCount; ++i) {
			auto drawList = &im3dContext->getDrawLists()[i];

			switch (drawList->m_primType) {
				case Im3d::DrawPrimitiveType::DrawPrimitive_Triangles:
					if (currentPipelineState != pipeline.mPipelineStateTriangles) {
						currentPipelineState = pipeline.mPipelineStateTriangles;
						deviceContext->SetPipelineState(currentPipelineState);
						deviceContext->CommitShaderResources(pipeline.mTriangleSRB, 
							DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
					}
					break;
				case Im3d::DrawPrimitive_Lines:
					if (currentPipelineState != pipeline.mPipelineStateLines) {
						currentPipelineState = pipeline.mPipelineStateLines;
						deviceContext->SetPipelineState(currentPipelineState);
						deviceContext->CommitShaderResources(pipeline.mLinesSRB, 
							DG::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
					}
					break;
				case Im3d::DrawPrimitive_Points:
					if (currentPipelineState != pipeline.mPipelineStateVertices) {
						currentPipelineState = pipeline.mPipelineStateVertices;
						deviceContext->SetPipelineState(currentPipelineState);
						deviceContext->CommitShaderResources(pipeline.mVertexSRB, 
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