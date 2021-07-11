#pragma once

#include <Engine/Raytrace/Raytracer.hpp>
#include <Engine/Resources/Texture.hpp>
#include <Engine/Resources/TextureIterator.hpp>

namespace Morpheus::Raytrace {
	class DefaultRaytracer : public IRaytraceDevice {
	private:
		TextureIterator mTextureIterator;
		ParameterizedTaskGroup<RenderParams> mRenderGroup;

		void Render(Frame* frame);

	public:

		Task Startup(SystemCollection& systems) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;

		IShape* CreateStaticMeshShape(const Geometry* rawGeo) override;
		void SetOutput(Texture* output) override;
	};
}