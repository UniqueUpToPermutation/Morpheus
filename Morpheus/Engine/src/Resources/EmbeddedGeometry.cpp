#include <Engine/Resources/Geometry.hpp>

namespace matball {
	#include <embed/matballmesh.hpp>
}

namespace box {
	#include <embed/boxmesh.hpp>
}

namespace bunny {
	#include <embed/bunnymesh.hpp>
}

namespace monkey {
	#include <embed/monkeymesh.hpp>
}

namespace plane {
	#include <embed/planemesh.hpp>
}

namespace sphere {
	#include <embed/spheremesh.hpp>
}

namespace teapot {
	#include <embed/teapotmesh.hpp>
}

namespace torus {
	#include <embed/torusmesh.hpp>
}

namespace Morpheus {

	RawGeometry RawGeometry::Prefabs::MaterialBall(const VertexLayout& layout) {
		RawGeometry geo;
		geo.FromMemory(layout, 
			matball::mVertexCount,
			matball::mIndexCount,
			matball::mIndices,
			matball::mPositions,
			matball::mUVs,
			matball::mNormals,
			matball::mTangents,
			matball::mBitangents);
		return geo;
	}

	RawGeometry RawGeometry::Prefabs::Box(const VertexLayout& layout) {
		RawGeometry geo;
		geo.FromMemory(layout, 
			box::mVertexCount,
			box::mIndexCount,
			box::mIndices,
			box::mPositions,
			box::mUVs,
			box::mNormals,
			box::mTangents,
			box::mBitangents);
		return geo;
	}

	RawGeometry RawGeometry::Prefabs::Sphere(const VertexLayout& layout) {
		RawGeometry geo;
		geo.FromMemory(layout, 
			sphere::mVertexCount,
			sphere::mIndexCount,
			sphere::mIndices,
			sphere::mPositions,
			sphere::mUVs,
			sphere::mNormals,
			sphere::mTangents,
			sphere::mBitangents);
		return geo;
	}

	RawGeometry RawGeometry::Prefabs::BlenderMonkey(const VertexLayout& layout) {
		RawGeometry geo;
		geo.FromMemory(layout, 
			monkey::mVertexCount,
			monkey::mIndexCount,
			monkey::mIndices,
			monkey::mPositions,
			monkey::mUVs,
			monkey::mNormals,
			monkey::mTangents,
			monkey::mBitangents);
		return geo;
	}

	RawGeometry RawGeometry::Prefabs::Torus(const VertexLayout& layout) {
		RawGeometry geo;
		geo.FromMemory(layout, 
			torus::mVertexCount,
			torus::mIndexCount,
			torus::mIndices,
			torus::mPositions,
			torus::mUVs,
			torus::mNormals,
			torus::mTangents,
			torus::mBitangents);
		return geo;
	}

	RawGeometry RawGeometry::Prefabs::Plane(const VertexLayout& layout) {
		RawGeometry geo;
		geo.FromMemory(layout, 
			plane::mVertexCount,
			plane::mIndexCount,
			plane::mIndices,
			plane::mPositions,
			plane::mUVs,
			plane::mNormals,
			plane::mTangents,
			plane::mBitangents);
		return geo;
	}

	RawGeometry RawGeometry::Prefabs::StanfordBunny(const VertexLayout& layout) {
		RawGeometry geo;
		geo.FromMemory(layout, 
			bunny::mVertexCount,
			bunny::mIndexCount,
			bunny::mIndices,
			bunny::mPositions,
			bunny::mUVs,
			bunny::mNormals,
			bunny::mTangents,
			bunny::mBitangents);
		return geo;
	}

	RawGeometry RawGeometry::Prefabs::UtahTeapot(const VertexLayout& layout) {
		RawGeometry geo;
		geo.FromMemory(layout, 
			teapot::mVertexCount,
			teapot::mIndexCount,
			teapot::mIndices,
			teapot::mPositions,
			teapot::mUVs,
			teapot::mNormals,
			teapot::mTangents,
			teapot::mBitangents);
			return geo;
	}
}