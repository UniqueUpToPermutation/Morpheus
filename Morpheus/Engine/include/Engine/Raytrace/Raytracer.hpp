#pragma once

#include <Engine/GeometryStructures.hpp>
#include <Engine/Renderer.hpp>
#include <Engine/Resources/Geometry.hpp>

namespace Morpheus::Raytrace {

	typedef float Float;

	struct SurfaceInteraction;

	class IShape {
	public:
		virtual bool RayIntersect(const Ray& ray, SurfaceInteraction* output, Float* t) const = 0;

		virtual BoundingBox Bounds() const = 0;

		inline bool RayIntersect(const Ray &ray) const {
			return RayIntersect(ray, nullptr, nullptr);
		}

		virtual ~IShape() = default;
	};

	struct Interaction {
		DG::float3 mP;
		Float mTime;
		DG::float3 mPError;
		DG::float3 mWo;
		DG::float3 mN;
	};

	struct SurfaceInteraction : public Interaction {
		DG::float2 mUV;
		DG::float3 mDPDU, mDPDV;
		DG::float3 mDNDU, mDNDV;
		
		const IShape *mShape = nullptr;

		struct {
			DG::float3 mN;
			DG::float3 mDPDU, mDPDV;
			DG::float3 mDNDU, mDNDV;
		} mShading;

		mutable DG::float3 mDPDX, mDPDY;
		mutable Float mDUDX = 0, mDVDX = 0, mDUDY = 0, mDVDY = 0;
	};

	class IRaytraceDevice {
	public:
		virtual IShape* CreateStaticMeshShape(const Geometry* rawGeo) = 0;
	};
}