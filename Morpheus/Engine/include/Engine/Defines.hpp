#pragma once

#include "BasicMath.hpp"

namespace DG = Diligent;

#ifdef PLATFORM_WIN32
typedef unsigned int uint;
#endif

namespace cereal {
	class PortableBinaryOutputArchive;
	class PortableBinaryInputArchive;
	class access;
}

namespace Diligent {
	class IRenderDevice;
	class IDeviceContext;
	class IFence;
}

namespace Diligent {
	template <class Archive>
	void serialize(Archive& archive,
		float2& m) {
		archive(m.x);
		archive(m.y);
	}

	template <class Archive>
	void serialize(Archive& archive,
		float3& m) {
		archive(m.x);
		archive(m.y);
		archive(m.z);
	}

	template <class Archive>
	void serialize(Archive& archive,
		float4& m) {
		archive(m.x);
		archive(m.y);
		archive(m.z);
		archive(m.w);
	}

	template <class Archive>
	void serialize(Archive& archive,
		Quaternion& q) {
		archive(q.q);		
	}
}
namespace Morpheus {

	class IDependencyResolver;

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

	template <typename T>
	struct ResourceComponent;

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
	class Material;

	template <typename T>
	struct LoadParams;

	class IResource;
	class ResourceCache;

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

	struct SerializationSet;

	struct ArchiveBlobPointer;
	class FrameHeader;

	constexpr MaterialId NullMaterialId = -1;
	constexpr ExtObjectId NullExtObjectId = -1;
};

using float2 = DG::float2;
using float3 = DG::float3;
using float4 = DG::float4;
using float2x2 = DG::float2x2;
using float3x3 = DG::float3x3;
using float4x4 = DG::float4x4;

namespace Assimp {
	class Importer;
}

struct aiScene;