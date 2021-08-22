#pragma once

#include "BasicMath.hpp"

namespace DG = Diligent;

#ifdef PLATFORM_WIN32
typedef unsigned int uint;
#endif

namespace cereal {
	class PortableBinaryOutputArchive;
	class PortableBinaryInputArchive;
}

namespace Diligent {
	class IRenderDevice;
	class IDeviceContext;
	class IFence;
}
namespace Morpheus {
	class ThreadPool;
	class TaskSyncPoint;
	struct TaskParams;

	template <typename T>
	class Future;

	template <typename T>
	class UniqueFuture;

	template <typename T>
	class Promise;

	class IComputeQueue;
	class ImmediateComputeQueue;
	class ThreadPool;

	class Engine;
	class EngineApp;

	class PlatformGLFW;

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

	struct Device;

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

	class IInterfaceCollection;
	class IResourceCacheCollection;

	struct SerializationSet;

	struct ArchiveBlobPointer;
	struct ResourceTableEntry;
	class FrameHeader;

	constexpr MaterialId NullMaterialId = -1;
	constexpr ExtObjectId NullExtObjectId = -1;
};

namespace Assimp {
	class Importer;
}

struct aiScene;