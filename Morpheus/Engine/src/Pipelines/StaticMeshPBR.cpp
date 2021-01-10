#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/Resources/PipelineResource.hpp>

namespace Morpheus {
	void CreateStaticMeshPBRPipeline(DG::IRenderDevice* device,
		ResourceManager* manager,
		IRenderer* renderer,
		ShaderLoader* shaderLoader,
		PipelineResource* into,
		const ShaderPreprocessorConfig* overrides) {
			
		ShaderPreprocessorConfig newOverrides;
		if (overrides) {
			newOverrides.mDefines = overrides->mDefines;
		}

		bool bUseSH = renderer->GetUseSHIrradiance();
		bool bUseIBL = renderer->GetUseIBL();

		auto itUseIBL = newOverrides.mDefines.find("USE_IBL");
		if (itUseIBL != newOverrides.mDefines.end()) {
			if (itUseIBL->second == "0" || itUseIBL->second == "false") {
				bUseIBL = false;
			} else if (itUseIBL->second == "1" || itUseIBL->second == "true") {
				bUseIBL = true;
			} else {
				throw std::runtime_error("USE_IBL macro has invalid value!");
			}
		} else {
			newOverrides.mDefines["USE_IBL"] = bUseIBL ? "1" : "0";
		}

		auto itUseSH = newOverrides.mDefines.find("USE_SH");
		if (itUseSH != newOverrides.mDefines.end()) {
			if (itUseSH->second == "0" || itUseSH->second == "false") {
				bUseSH = false;
			} else if (itUseSH->second == "1" || itUseSH->second == "true") {
				bUseSH = true;
			} else {
				throw std::runtime_error("USE_IBL macro has invalid value!");
			}
		} else {
			newOverrides.mDefines["USE_SH"] = bUseSH ? "1" : "0";
		}

		auto pbrStaticMeshVS = LoadShader(device, 
			DG::SHADER_TYPE_VERTEX,
			"internal/StaticMesh.vsh",
			"StaticMesh VS",
			"main",
			&newOverrides,
			shaderLoader);

		auto pbrStaticMeshPS = LoadShader(device,
			DG::SHADER_TYPE_PIXEL,
			"internal/PBR.psh",
			"PBR PS",
			"main",
			&newOverrides,
			shaderLoader);

		auto anisotropyFactor = renderer->GetMaxAnisotropy();
		auto filterType = anisotropyFactor > 1 ? DG::FILTER_TYPE_ANISOTROPIC : DG::FILTER_TYPE_LINEAR;

		DG::SamplerDesc SamLinearClampDesc
		{
			filterType, filterType, filterType, 
			DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP, DG::TEXTURE_ADDRESS_CLAMP
		};

		DG::SamplerDesc SamLinearWrapDesc 
		{
			filterType, filterType, filterType, 
			DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_WRAP, DG::TEXTURE_ADDRESS_WRAP
		};

		SamLinearClampDesc.MaxAnisotropy = anisotropyFactor;
		SamLinearWrapDesc.MaxAnisotropy = anisotropyFactor;

		DG::IPipelineState* result = nullptr;

		// Create Irradiance Pipeline
		DG::GraphicsPipelineStateCreateInfo PSOCreateInfo;
		DG::PipelineStateDesc&              PSODesc          = PSOCreateInfo.PSODesc;
		DG::GraphicsPipelineDesc&           GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

		PSODesc.Name         = "Static Mesh PBR Pipeline";
		PSODesc.PipelineType = DG::PIPELINE_TYPE_GRAPHICS;

		GraphicsPipeline.NumRenderTargets             = 1;
		GraphicsPipeline.RTVFormats[0]                = renderer->GetIntermediateFramebufferFormat();
		GraphicsPipeline.PrimitiveTopology            = DG::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		GraphicsPipeline.RasterizerDesc.CullMode      = DG::CULL_MODE_BACK;
		GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
		GraphicsPipeline.DepthStencilDesc.DepthFunc   = DG::COMPARISON_FUNC_LESS;
		GraphicsPipeline.DSVFormat 					  = renderer->GetIntermediateDepthbufferFormat();

		// Number of MSAA samples
		GraphicsPipeline.SmplDesc.Count = renderer->GetMSAASamples();

		uint stride = 12 * sizeof(float);

		std::vector<DG::LayoutElement> layoutElements = {
			DG::LayoutElement(0, 0, 3, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(1, 0, 3, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(2, 0, 2, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),
			DG::LayoutElement(3, 0, 3, DG::VT_FLOAT32, false, 
				DG::LAYOUT_ELEMENT_AUTO_OFFSET, stride, DG::INPUT_ELEMENT_FREQUENCY_PER_VERTEX),

			DG::LayoutElement(4, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(5, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(6, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE),
			DG::LayoutElement(7, 1, 4, DG::VT_FLOAT32, false, DG::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE)
		};

		GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
		GraphicsPipeline.InputLayout.LayoutElements = &layoutElements[0];

		PSOCreateInfo.pVS = pbrStaticMeshVS;
		PSOCreateInfo.pPS = pbrStaticMeshPS;

		PSODesc.ResourceLayout.DefaultVariableType = DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
		
		std::vector<DG::ShaderResourceVariableDesc> Vars;
		
		// clang-format off
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"mAlbedo", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"mMetallic", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"mRoughness", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});
		Vars.emplace_back(DG::ShaderResourceVariableDesc{
			DG::SHADER_TYPE_PIXEL, 
			"mNormalMap", 
			DG::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC});

		if (bUseIBL) {
			if (bUseSH) {
				Vars.emplace_back(DG::ShaderResourceVariableDesc{
					DG::SHADER_TYPE_PIXEL, 
					"IrradianceSH", 
					DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});
			}
			else {
				Vars.emplace_back(DG::ShaderResourceVariableDesc{
					DG::SHADER_TYPE_PIXEL, 
					"mIrradianceMap", 
					DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});
			}
			Vars.emplace_back(DG::ShaderResourceVariableDesc{
				DG::SHADER_TYPE_PIXEL, 
				"mPrefilteredEnvMap", 
				DG::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE});
				// clang-format off
			Vars.emplace_back(DG::ShaderResourceVariableDesc{
				DG::SHADER_TYPE_PIXEL, 
				"mBRDF_LUT", 
				DG::SHADER_RESOURCE_VARIABLE_TYPE_STATIC});
		}

		// clang-format on
		PSODesc.ResourceLayout.NumVariables = Vars.size();
		PSODesc.ResourceLayout.Variables    = &Vars[0];

		std::vector<DG::ImmutableSamplerDesc> ImtblSamplers;

		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "mAlbedo_sampler", SamLinearWrapDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "mRoughness_sampler", SamLinearWrapDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "mMetallic_sampler", SamLinearWrapDesc
		});
		ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
			DG::SHADER_TYPE_PIXEL, "mNormalMap_sampler", SamLinearWrapDesc
		});

		if (bUseIBL) {
			if (!bUseSH) {
				ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
					DG::SHADER_TYPE_PIXEL, "mIrradianceMap_sampler", SamLinearClampDesc
				});
			}
			ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
				DG::SHADER_TYPE_PIXEL, "mPrefilteredEnvMap_sampler", SamLinearClampDesc
			});
			ImtblSamplers.emplace_back(DG::ImmutableSamplerDesc{
				DG::SHADER_TYPE_PIXEL, "mBRDF_LUT_sampler", SamLinearClampDesc
			});
		}

		// clang-format on
		PSODesc.ResourceLayout.NumImmutableSamplers = ImtblSamplers.size();
		PSODesc.ResourceLayout.ImmutableSamplers    = &ImtblSamplers[0];
		
		device->CreateGraphicsPipelineState(PSOCreateInfo, &result);

		auto globalsVar =result->GetStaticVariableByName(DG::SHADER_TYPE_VERTEX, "Globals");
		if (globalsVar)
			globalsVar->Set(renderer->GetGlobalsBuffer());
		globalsVar = result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "Globals");
		if (globalsVar)
			globalsVar->Set(renderer->GetGlobalsBuffer());

		if (bUseIBL) {
			auto lutVar = result->GetStaticVariableByName(DG::SHADER_TYPE_PIXEL, "mBRDF_LUT");
			if (lutVar)
				lutVar->Set(renderer->GetLUTShaderResourceView());
		}

		pbrStaticMeshVS->Release();
		pbrStaticMeshPS->Release();

		VertexAttributeLayout layout;
		layout.mPosition = 0;
		layout.mNormal = 1;
		layout.mUV = 2;
		layout.mTangent = 3;
		layout.mStride = 12 * sizeof(float);

		into->SetAll(
			result,
			layoutElements,
			layout,
			InstancingType::INSTANCED_STATIC_TRANSFORMS);
	}
}