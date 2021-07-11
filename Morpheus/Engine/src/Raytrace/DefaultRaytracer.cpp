#include <Engine/Raytrace/DefaultRaytracer.hpp>

namespace Morpheus::Raytrace {
	void DefaultRaytracer::Render(Frame* frame) {

		if (!mTextureIterator.IsValid())
			throw std::runtime_error(
				"Raytracer output is not set! Use SetOutput to "
				"set the raytracer's target texture!");

		for (auto it = mTextureIterator; it.IsValid(); it.Next()) {
			DG::float4 value;

			auto uv = it.Position();

			value.r = uv.x;
			value.g = uv.y;
			value.b = 1.0;
			value.a = 1.0;

			it.Value().Write(value);
		}
	}

	Task DefaultRaytracer::Startup(SystemCollection& systems) {
		ParameterizedTask<RenderParams> renderTask(
			[this](const TaskParams& params, const RenderParams& e) {
				Render(e.mFrame);
			}, "Raytracer Render", TaskType::RENDER
		);

		systems.AddRenderTask(std::move(renderTask));
		return Task();
	}

	bool DefaultRaytracer::IsInitialized() const {
		return true;
	}

	void DefaultRaytracer::Shutdown() {
	}

	void DefaultRaytracer::NewFrame(Frame* frame) {
	}

	void DefaultRaytracer::OnAddedTo(SystemCollection& collection) {
	}

	IShape* DefaultRaytracer::CreateStaticMeshShape(const Geometry* rawGeo) {
		return nullptr;
	}
	
	void DefaultRaytracer::SetOutput(Texture* output) {

		if (!output->IsRaw()) 
			throw std::runtime_error("Output texture must have raw aspect!");

		mTextureIterator = TextureIterator(output);
	}
}