#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "BasicMath.hpp"

#include <Engine/Graphics.hpp>
#include <Engine/Components/LightProbe.hpp>
#include <Engine/Resources/Shader.hpp>

namespace DG = Diligent;

#define DEFAULT_LUT_SURFACE_ANGLE_SAMPLES 512
#define DEFAULT_LUT_ROUGHNESS_SAMPLES 512
#define DEFAULT_LUT_INTEGRATION_SAMPLES 512

namespace Morpheus {
	class CookTorranceLUT {
	private:
		Texture mLut;

	public:
		void Compute(DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			uint surfaceAngleSamples = DEFAULT_LUT_SURFACE_ANGLE_SAMPLES, 
			uint roughnessSamples = DEFAULT_LUT_ROUGHNESS_SAMPLES,
			uint integrationSamples = DEFAULT_LUT_INTEGRATION_SAMPLES);

		inline DG::ITexture* GetLUT() {
			return mLut.GetRasterTexture();
		}

		inline DG::ITextureView* GetShaderView() {
			return mLut.GetShaderView();
		}

		void SavePng(const std::string& path, DG::IDeviceContext* context, DG::IRenderDevice* device);
	};

	struct PrecomputeEnvMapAttribs
    {
        DG::float4x4 Rotation;

        float Roughness;
        float EnvMapDim;
        uint  NumSamples;
        float Dummy;
    };

	struct LightProbeProcessorConfig {
		uint mIrradianceSHSamples = 5000;
		bool bEnvMapOptimizeSamples = true;
		uint mEnvMapSamples = 256;
		DG::TEXTURE_FORMAT mPrefilteredEnvFormat;
	};

	struct LightProbeProcessorShaders {
		Handle<DG::IShader> mPrefilterEnvVS;
		Handle<DG::IShader> mPrefilterEnvPS;
		Handle<DG::IShader> mSHShaderCS;

		static Future<LightProbeProcessorShaders> Load(
			DG::IRenderDevice* device,
			const LightProbeProcessorConfig& config,
			IVirtualFileSystem* fileSystem = EmbeddedFileLoader::GetGlobalInstance());
	};

	class LightProbeProcessor {
	private:
		Handle<DG::IPipelineState> mPrefilterEnvPipeline;
		Handle<DG::IPipelineState> mSHIrradiancePipeline;
		Handle<DG::IBuffer> mTransformConstantBuffer;
		Handle<DG::IShaderResourceBinding> mPrefilterEnvSRB;
		Handle<DG::IShaderResourceBinding> mSHIrradianceSRB;
		LightProbeProcessorConfig mConfig;

	public:
		LightProbeProcessor(DG::IRenderDevice* device, 
			const LightProbeProcessorShaders& shaders,
			const LightProbeProcessorConfig& config);

		void ComputeIrradiance(
			DG::IDeviceContext* context,
			DG::ITextureView* incommingEnvironmentSRV,
			DG::IBufferView* outputBufferView);

		DG::IBuffer* ComputeIrradiance(
			DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			DG::ITextureView* incommingEnvironmentSRV
		);

		void ComputePrefilteredEnvironment(
			DG::IDeviceContext* context, 
			DG::ITextureView* incommingEnvironmentSRV,
			DG::ITexture* outputCubemap);

		DG::ITexture* ComputePrefilteredEnvironment(
			DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			DG::ITextureView* incommingEnvironmentSRV,
			uint size);

		LightProbe ComputeLightProbe(
			DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			DG::ITextureView* incommingEnvironmentSRV,
			uint prefilteredEnvironmentSize = 256);
	};
}