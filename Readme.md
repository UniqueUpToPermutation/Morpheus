# Morpheus Graphics Engine

**NOTE: The Engine is currently undergoing an overhaul. While I'm working on the overhaul, the details below may be inaccurate.**

Morpheus is a realtime graphics engine built ontop of the low-level [DiligentEngine](https://github.com/DiligentGraphics/DiligentEngine) graphics API. Morpheus comes with a physically based renderer with spherical harmonics based irradiance. Morpheus is currently under development and is intended mostly as an educational hobby project.

![Gun1](images/gun1.png)

![Gun2](images/gun2.png)

The [original version](https://github.com/UniqueUpToPermutation/Morpheus) of this engine was written in OpenGL, but I have migrated to DiligentEngine because I've run into a number of limitations with OpenGL, and Vulkan is too verbose for my liking. DiligentEngine is a thin abstraction layer on top of Direct3D12/11, Vulkan, Metal and OpenGL, and I've quite enjoyed working with it and recommend it for other who are interested.

## Features

Everything in my graphics engine is currently a work in progress. However, there are a number of features that are currently in the engine:

- **Physically-Based Forward Renderer** (see **DefaultRenderer** class): the shading model underlying the renderer is based off the one used in [Unreal Engine 4](https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf). For efficiently representing the irradiance field, I compute and transform spherical harmonics coefficients via the technique in [Ramamoorthi et al.](https://cseweb.ucsd.edu/~ravir/papers/envmap/envmap.pdf)
- **Thread Pool/Task Scheduler**: The engine includes a thread pool and task scheduler for asyncronous and parallel computation. The design was inspired by [Naughty Dog's GDC Talk](https://www.gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine), but I don't use fibers.
- **Resource Manager** (see **ResourceManager** class): The engine includes a resource manager that allows the user to load the following classes of content:
  - *Shaders*: Shader programs can be loaded from embedded HLSL code. I have built a shader preprocessor that resolves #include directives appropriately and emplaces proper #line directives.
  - *Pipelines*: Rendering in DiligentEngine revolves around monolithic pipeline objects that specify how data is processed by the GPU. I provide a few defaults (such as a physically-based static mesh pipeline), and more can be added via the internal pipeline factory.
  - *Textures*: Textures are loaded either via *lodepng* (png), *stb* (hdr), or *gli* (ktx or dds). One should prefer *gli* because the format includes mipmaps.
  - *Geometry*: Geometry is loaded via the *assimp* library. For any piece of geometry, the resource manager will request a corresponding pipeline to determine the vertex format.
  - *Materials*: Materials are implemented as shader resource bindings, they can have views attached to them that allow them to expose certain functionality (i.e., for environmental lighting etc.)
  - Resources are reference counted so that they can be released when not longer needed.
  - **Resources can be streamed into GPU memory asynchronously via the task scheduler**.
- **Entity-Component-System Design**: The engine uses [ENTT](https://github.com/skypjack/entt) to provide a data-centric approach to scene management. Scenes in my engine are simply a pure collection of data.
- **Realtime Physics**: implemented via the [Bullet Physics Engine](https://github.com/bulletphysics/bullet3). Integration is still a work in progress, but some limited rigid body functionality is available.
- **Cross-Platform for Linux and Windows**: The engine runs on both Linux and Windows currently. MacOS, Android, UWP, and iOS are possibilities but are not high priority right now.

## Supported Platforms

I am currently supporting Linux (via **gcc**) and Windows (via **msvc**). Although technically supported, Windows is not a priority for me, as I prefer to work in Linux when possible, but graphics drivers on Windows tend to perform significantly better -- so I'll put more effort into that platform later down the line.

## Dependencies

All of these dependencies, except for **assimp** are included as submodules.

- [**assimp**](https://github.com/assimp/assimp): Used to load geometry. Install separately. On Windows, I recommend using **vcpkg**.
- [**DiligentEngine**](https://github.com/DiligentGraphics/DiligentEngine): Used for graphics rendering.
- [**Bullet Physics**](https://github.com/bulletphysics/bullet3): Used for physics.
- [**lodepng**](https://github.com/lvandeve/lodepng): Used to load PNG texture files.
- [**gli**](https://github.com/g-truc/gli): Used to load KTX/DDS texture files.
- [**stb**](https://github.com/nothings/stb): Used to load high dynamic range images (HDR).
- [**nlohmann/json**](https://github.com/nlohmann/json): Used to load JSON files.
- [**entt**](https://github.com/skypjack/entt): Used for entity-component-system.
- **cmake**: Build system.

### Extra Windows Dependencies

- [**Windows SDK**](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/): Required for Win32 window creation and Direct3D11/12. 
- **Microsoft Visual Studio / Visual C++**: Probably the easiest way to build everything on Windows is to use cmake to output Microsoft Visual Studio solutions. 

## Build Instructions

Building in Linux should be straightforward, use **cmake** to generate Makefiles and then call **make**.

Building in Windows (as always) is harder. The easiest way to do it is to use **cmake** to output a Microsoft Visual Studio solution. You can then build the solution with Visual Studio. For some reason, the Visual Studio fails to do a partial compile of the engine. Still looking into this.

### Output Directories

One thing to note is if you are building with the cmake extension for VSCode, it is useful to have different output directories for different build configurations:

```json
"cmake.buildDirectory" : "${workspaceRoot}/build/${buildType}"
```

### Running in VSCode under Linux

I use VSCode as my development environment, and I prefer to work on Linux.

However, in Linux, the environment variable $DISPLAY must be set because OpenGL/Vulkan uses it to spawn a window and graphics device from an X11 display server. You can add the following code to your .bashrc:

```bash
if [ -z "$DISPLAY" ]
then
        export DISPLAY=":1"
fi
```

And then you can set the following in your VSCode config:

```json
"terminal.integrated.shellArgs.linux": ["-l"]
```

This will force VSCode to open its terminal as a login terminal, and run your .bashrc when it starts up.

# Examples

The engine currently includes the following examples:

## Physically Based Rendering

**Located in Morpheus/PBR**. This sample demonstrates the physically based forward renderer of the engine using a model from [this repository](https://github.com/Nadrin/PBR).

A static model is loaded into the world by calling the **ResourceManager::LoadMesh** function with a path to the model and to the material (as a JSON file) that model should use. Both the material and geometry can then be attached to gun node entity, together with a 3D transformation to determine the gun's position/rotation.

```C++
// Create Gun
GeometryResource* gunMesh;
MaterialResource* gunMaterial;
content->LoadMesh("cerberus.obj", "cerberusmat.json", 
	&gunMesh, &gunMaterial);

auto gunNode = root.CreateChild();
gunNode.Add<GeometryComponent>(gunMesh);
gunNode.Add<MaterialComponent>(gunMaterial);

Transform& transform = gunNode.Add<Transform>(
	DG::float3(0.0f, 0.0f, 0.0f),
	DG::Quaternion::RotationFromAxisAngle(
		DG::float3(0.0f, 1.0f, 0.0f), DG::PI),
	DG::float3(8.0f, 8.0f, 8.0f)
);

gunMaterial->Release();
gunMesh->Release();
```

To create the skybox, we load an HDRI and then convert it to a cubemap by using the engine's **HdriToCubemap** class. When the scene begins, the engine will automatically find the skybox entity and compute both global irradiance coefficients and a global specular environment map, which will then be attached to the skybox entity via a **LightProbe**.

```C++
// Load HDRI and convert it to a cubemap
auto skybox_hdri = en.GetResourceManager()->Load<TextureResource>("environment.hdr");
HDRIToCubemapConverter conv(en.GetDevice());
conv.Initialize(content, DG::TEX_FORMAT_RGBA16_FLOAT);
auto skybox_texture = conv.Convert(en.GetDevice(), en.GetImmediateContext(), skybox_hdri->GetShaderView(), 2048, true);
skybox_hdri->Release();

// Create skybox from HDRI cubemap
auto tex_res = new TextureResource(content, skybox_texture);
tex_res->AddRef();
auto skybox = root.CreateChild();
skybox.Add<SkyboxComponent>(tex_res);
tex_res->Release();
```

To enable camera movement, we attach a controller to the scene's camera entity:

```C++
// Create a controller 
auto cameraNode = scene->GetCameraNode();
cameraNode.Add<Transform>().SetTranslation(0.0f, 0.0f, -5.0f);
cameraNode.Add<EditorCameraController>(cameraNode, scene);
```

This gives us the following scene:

![Gun Image 1](images/gun1.png)

![Gun Image 1](images/gun2.png)

## Physics

**Located in Morpheus/Physics**. This sample demonstrates how to use the physics system. One must enable the system by adding it to a scene:

```C++
scene->AddSystem<PhysicsSystem>();
```

Afterwards, one can simply attach Bullet rigid bodies to entities, and the engine's physics system should automatically take care of the rest. For example, to create a ground tile, one can call:

```C++
// Create Bullet rigid body
btBoxShape* box = new btBoxShape(btVector3(10.0f, 0.1f, 10.0f));
btRigidBody* ground_rb = new btRigidBody(0.0f, nullptr, box);

// Attach the rigid body to a a visible entity in the world
GeometryResource* groundMesh;
MaterialResource* groundMaterial;
content->LoadMesh("ground.obj", "brick.json", &groundMesh, &groundMaterial);

auto groundNode = root.CreateChild();
groundNode.Add<GeometryComponent>(groundMesh);
groundNode.Add<MaterialComponent>(groundMaterial);
groundNode.Add<Transform>().SetTranslation(0.0f, -10.0f, 0.0f);
groundNode.Add<RigidBodyComponent>(ground_rb);

groundMesh->Release();
groundMaterial->Release();
```

The result is:

![PhysicsDemo](images/vid.gif)

## Asynchronous Resource Loading

**Located in Morpheus/AsyncTest**. This sample demonstrates how to stream resources from the disk asynchronously. The process is more or less the same as loading resources normally. However, the resources cannot be used until the barrier of the resource in question has been reached. Users can obtain the barrier of a resources via **IResource::GetLoadBarrier** use the barrier to query when the resource has been loaded or schedule additional tasks afterwards with the thread pool. Additionally, the user can pass a callback that is executed when the resource has been loaded.

```C++
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
```

## Sprite Batch

One can also render 2D entities via the **SpriteBatch** class. The sprite batch allows users to group together sprites to reduce the number of draw calls to the GPU that need to be made to render a collection of sprites. There is no sprite sorting functionality currently implemented. Such functionality should be implemented by a 2D renderer. The sprite batch uses the camera of a scene to render sprites. So, if you are making a 2D game, your camera should be orthographic, also make sure that your clipping planes include the value z = 0.

```C++
auto camera = scene->GetCamera();
camera->SetType(CameraType::ORTHOGRAPHIC);
camera->SetClipPlanes(-1.0, 1.0);
camera->SetOrthoSize(Width, Height);
```

Afterwards, you can load the texture and create a sprite batch:

```C++
TextureResource* spriteTexture = 
	en.GetResourceManager()->Load<TextureResource>("sprite.png");
std::unique_ptr<SpriteBatch> spriteBatch(
	new SpriteBatch(en.GetDevice(), en.GetResourceManager()));
```

Make sure that you update the globals constant buffer (a call to the **Render** function will do this), and afterwards, you can submit sprites to the sprite batch as such:

```C++
// Draw all sprites
spriteBatch->Begin(en.GetImmediateContext());

for (auto& obj : insts) {
	spriteBatch->Draw(spriteTexture, obj.mPosition,
		obj.mSize, obj.mOrigin, obj.mRotation, obj.mColor);
}

spriteBatch->End();
```

This example uses the sprite batch to draw a bunch of different colored sprites to the screen:

![SpriteBatchImg](images/vid2.gif)
