#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "BasicMath.hpp"

#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/LightProbe.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class CookTorranceLUT {
	private:
		DG::ITexture* mLut;

	public:
		CookTorranceLUT(DG::IRenderDevice* device, 
			uint SamplesPerPixel=512, 
			uint NdotVSamples=512);

		~CookTorranceLUT();

		void Compute(ResourceManager* resourceManager,
			DG::IDeviceContext* context, 
			DG::IRenderDevice* device,
			uint Samples=512);

		inline DG::ITexture* GetLUT() {
			return mLut;
		}

		inline DG::ITextureView* GetShaderView() {
			return mLut->GetDefaultView(DG::TEXTURE_VIEW_SHADER_RESOURCE);
		}

		void SavePng(const std::string& path, DG::IDeviceContext* context, DG::IRenderDevice* device);
		void SaveGli(const std::string& path, DG::IDeviceContext* context, DG::IRenderDevice* device);
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
		DG::IPipelineState* mIrradiancePipeline;
		DG::IPipelineState* mPrefilterEnvPipeline;
		DG::IPipelineState* mSHIrradiancePipeline;
		DG::IBuffer* mTransformConstantBuffer;
		DG::IShaderResourceBinding* mIrradianceSRB;
		DG::IShaderResourceBinding* mPrefilterEnvSRB;
		DG::IShaderResourceBinding* mSHIrradianceSRB;

		DG::TEXTURE_FORMAT mIrradianceFormat;
		DG::TEXTURE_FORMAT mPrefilteredEnvFormat;

		uint mEnvironmentMapSamples;
		uint mIrradianceSHSamples;

	public:
		LightProbeProcessor(DG::IRenderDevice* device);
		~LightProbeProcessor();

		void Initialize(
			ResourceManager* resourceManager,
			DG::TEXTURE_FORMAT irradianceFormat,
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
			DG::ITexture* outputCubemap);

		void ComputeIrradianceSH(
			DG::IDeviceContext* context,
			DG::ITextureView* incommingEnvironmentSRV,
			DG::IBufferView* outputBufferView);

		DG::IBuffer* ComputeIrradianceSH(
			DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			DG::ITextureView* incommingEnvironmentSRV
		);

		void ComputePrefilteredEnvironment(
			DG::IDeviceContext* context, 
			DG::ITextureView* incommingEnvironmentSRV,
			DG::ITexture* outputCubemap);

		DG::ITexture* ComputeIrradiance(
			DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			DG::ITextureView* incommingEnvironmentSRV,
			uint size);

		DG::ITexture* ComputePrefilteredEnvironment(
			DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			DG::ITextureView* incommingEnvironmentSRV,
			uint size);

		LightProbe ComputeLightProbe(
			DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			ResourceCache<TextureResource>* textureCache,
			DG::ITextureView* incommingEnvironmentSRV,
			uint prefilteredEnvironmentSize = 256,
			uint irradianceSize = 64);

		LightProbe ComputeLightProbeSH(
			DG::IRenderDevice* device,
			DG::IDeviceContext* context,
			ResourceCache<TextureResource>* textureCache,
			DG::ITextureView* incommingEnvironmentSRV,
			uint prefilteredEnvironmentSize = 256);
	};
}