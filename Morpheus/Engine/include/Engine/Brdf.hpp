#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "BasicMath.hpp"

#include <Engine/Graphics.hpp>
#include <Engine/LightProbe.hpp>

namespace DG = Diligent;

#define DEFAULT_LUT_SURFACE_ANGLE_SAMPLES 512
#define DEFAULT_LUT_ROUGHNESS_SAMPLES 512
#define DEFAULT_LUT_INTEGRATION_SAMPLES 512

namespace Morpheus {
	class CookTorranceLUT {
	private:
		Handle<DG::ITexture> mLut;

	public:
		void Compute(DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			uint surfaceAngleSamples = DEFAULT_LUT_SURFACE_ANGLE_SAMPLES, 
			uint roughnessSamples = DEFAULT_LUT_ROUGHNESS_SAMPLES,
			uint integrationSamples = DEFAULT_LUT_INTEGRATION_SAMPLES);

		inline DG::ITexture* GetLUT() {
			return mLut.Ptr();
		}

		inline DG::ITextureView* GetShaderView() {
			return mLut->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);
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
		uint mIrradianceSamplesTheta = 32;
		uint mIrradianceSamplesPhi = 64;
		uint mIrradianceSHSamples = 5000;
		bool bEnvMapOptimizeSamples = true;
		uint mEnvMapSamples = 256;
	};
	class LightProbeProcessor {
	private:
		DG::IPipelineState* mPrefilterEnvPipeline;
		DG::IPipelineState* mSHIrradiancePipeline;
		DG::IBuffer* mTransformConstantBuffer;
		DG::IShaderResourceBinding* mPrefilterEnvSRB;
		DG::IShaderResourceBinding* mSHIrradianceSRB;

		DG::TEXTURE_FORMAT mPrefilteredEnvFormat;

		uint mEnvironmentMapSamples;
		uint mIrradianceSHSamples;

	public:
		LightProbeProcessor(DG::IRenderDevice* device);
		~LightProbeProcessor();

		void Initialize(
			DG::IRenderDevice* device,
			DG::TEXTURE_FORMAT prefilterEnvFormat,
			const LightProbeProcessorConfig& config = LightProbeProcessorConfig());

		inline void SetEnvironmentMapSamples(uint envMapSamples) {
			mEnvironmentMapSamples = envMapSamples;
		}

		inline uint GetEnvironmentMapSamples() const {
			return mEnvironmentMapSamples;
		}

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