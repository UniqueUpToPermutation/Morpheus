#include <Engine/Engine.hpp>
#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Resources/MaterialResource.hpp>
#include <Engine/Resources/GeometryResource.hpp>
#include <Engine/Components/ResourceComponents.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/HdriToCubemap.hpp>
#include <Engine/Components/SkyboxComponent.hpp>
#include <Engine/Brdf.hpp>
#include <Engine/EditorCameraController.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Resources/ShaderResource.hpp>
#include <Engine/Systems/Physics.hpp>
#include <Engine/ThreadTasks.hpp>

using namespace Morpheus;

std::mutex mOutput;

int main(int argc, char** argv) {
	Engine en;

	en.Startup(argc, argv);

	Scene* scene = new Scene();

	auto manager = en.GetResourceManager();

	PipelineResource* pipeline = manager->AsyncLoad<PipelineResource>("White", [](ThreadPool* pool) {
		std::cout << "Loaded pipeline!" << std::endl;
	});

	TextureResource* texture = manager->AsyncLoad<TextureResource>("brick_albedo.png", [](ThreadPool* pool) {
		std::cout << "Loaded texture!" << std::endl;
	});

	MaterialResource* material = manager->AsyncLoad<MaterialResource>("material.json", [](ThreadPool* pool) {
		std::cout << "Loaded material!" << std::endl;
	});

	TextureResource* hdrTexture = manager->AsyncLoad<TextureResource>("environment.hdr", [](ThreadPool* pool) {
		std::cout << "Loaded HDR texture!" << std::endl;
	});

	en.InitializeDefaultSystems(scene);
	scene->Begin();

	en.CollectGarbage();

	while (en.IsReady()) {
		en.YieldFor(std::chrono::milliseconds(10));
		en.Update(scene);
		en.Render(scene);
		en.RenderUI();
		en.Present();
	}

	texture->Release();
	pipeline->Release();
	material->Release();
	hdrTexture->Release();

	en.Shutdown();
}