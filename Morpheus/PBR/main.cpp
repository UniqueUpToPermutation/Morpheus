#include <Engine/Core.hpp>
#include <Engine/HdriToCubemap.hpp>
#include <Engine/DefaultRenderer.hpp>

using namespace Morpheus;


#if PLATFORM_WIN32
int __stdcall WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd) {
#endif

#if PLATFORM_LINUX
int main(int argc, char** argv) {
#endif
	
	Engine en;

	en.AddComponent<DefaultRenderer>();
	en.Startup();

	Scene* scene = new Scene();

	auto root = scene->GetRoot();
	auto content = en.GetResourceManager();

	// Create Gun
	GeometryResource* gunMesh;
	MaterialResource* gunMaterial;
	content->LoadMesh("cerberus.obj", "cerberusmat.json", &gunMesh, &gunMaterial);
	
	auto gunNode = root.CreateChild();
	gunNode.Add<GeometryComponent>(gunMesh);
	gunNode.Add<MaterialComponent>(gunMaterial);

	Transform& transform = gunNode.Add<Transform>(
		DG::float3(0.0f, 0.0f, 0.0f),
		DG::Quaternion::RotationFromAxisAngle(DG::float3(0.0f, 1.0f, 0.0f), DG::PI),
		DG::float3(8.0f, 8.0f, 8.0f)
	);

	gunMaterial->Release();
	gunMesh->Release();

	// Load HDRI and convert it to a cubemap
	auto skybox_hdri = en.GetResourceManager()->Load<TextureResource>("environment.hdr");
	HDRIToCubemapConverter conv(en.GetDevice());
	conv.Initialize(en.GetDevice(), DG::TEX_FORMAT_RGBA16_FLOAT);
	auto skybox_texture = conv.Convert(en.GetDevice(), en.GetImmediateContext(), skybox_hdri->GetShaderView(), 2048, true);
	skybox_hdri->Release();

	// Create skybox from HDRI cubemap
	auto tex_res = new TextureResource(content, skybox_texture);
	tex_res->AddRef();
	auto skybox = root.CreateChild();
	skybox.Add<SkyboxComponent>(tex_res);
	tex_res->Release();

	// Create a controller 
	auto cameraNode = scene->GetCameraNode();
	cameraNode.Add<Transform>().SetTranslation(0.0f, 0.0f, -5.0f);
	cameraNode.Add<EditorCameraController>(cameraNode, scene);

	en.InitializeDefaultSystems(scene);
	scene->Begin();

	en.CollectGarbage();

	while (en.IsReady()) {
		en.Update(scene);
		en.Render(scene);
		en.RenderUI();
		en.Present();
	}

	delete scene;
	en.Shutdown();
}