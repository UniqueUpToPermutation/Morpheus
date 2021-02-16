#pragma once

#include <Engine/Resources/CollisionShapeResource.hpp>

#include <Engine/Components/UniquePointerComponent.hpp>
#include <Engine/Components/RefCountComponent.hpp>

#include "btBulletDynamicsCommon.h"

namespace Morpheus {
	typedef RefCountComponent<CollisionShapeResource> CollisionShapeComponent;
	typedef UniquePointerComponent<btRigidBody> RigidBodyComponent;
}