#pragma once

#include <btBulletDynamicsCommon.h>

#include <Engine/Resources/Resource.hpp>

namespace Morpheus {
	class CollisionShapeResource : public IResource {
	public:
		btCollisionShape* mShape;
	};
}