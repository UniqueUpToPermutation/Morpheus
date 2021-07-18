#pragma once

#include "BasicMath.hpp"

namespace DG = Diligent;

#ifdef PLATFORM_WIN32
typedef unsigned int uint;
#endif

namespace Morpheus {
	class ThreadPool;
	class TaskSyncPoint;
	struct TaskParams;
	class ITaskQueue;
	class TaskNodeOut;
	class TaskNodeIn;
	class ITask;
	class TaskGroup;
	struct TaskBarrier;
	struct Task;

	template <typename T>
	class Future;

	template <typename T>
	class Promise;

	class ITaskQueue;
	class ImmediateTaskQueue;
	class ThreadPool;

	class Engine;
	class EngineApp;

	class PlatformLinux;
	class PlatformWin32;

	struct PlatformParams;

	class IPlatform;
	class Platform;

	template <typename T>
	class DynamicGlobalsBuffer;

	class Camera;

	class Scene;
	struct HierarchyData;

	class DepthFirstNodeIterator;
	class DepthFirstNodeDoubleIterator;
	struct Frame;

	struct BoundingBox;
	struct BoundingBox2D;
	struct SpriteRect;
	struct Ray;
	struct VertexLayout;

	class IVertexFormatProvider;

	struct GraphicsDevice;

	class IRenderer;
	struct GraphicsParams;
	class RealtimeGraphics;

	class IRenderer;
	struct MaterialDesc;
	struct MaterialDescFuture;
	class Material;
	class IRenderer;

	class SpriteBatchGlobals;
	class SpriteBatch;

	class Texture;
	class Geometry;

	template <typename T>
	struct LoadParams;

	class IResource;

	template <typename T>
	class Handle;

	class RawShader;

	class IRenderer;
	class ISystem;
	class IExternalGraphicsDevice;

	enum class ExtObjectType {
		TEXTURE,
		GEOMETRY
	};

	typedef int32_t ExtObjectId;
	typedef int32_t MaterialId;

	constexpr MaterialId NullMaterialId = -1;
	constexpr ExtObjectId NullExtObjectId = -1;
};

namespace Assimp {
	class Importer;
}

struct aiScene;