#pragma once

#include <Engine/Resources/CollisionShapeResource.hpp>

#include <Engine/Components/UniquePointerComponent.hpp>

#include "btBulletDynamicsCommon.h"

namespace Morpheus {
	typedef RefHandle<CollisionShapeResource> CollisionShapeComponent;
	typedef UniquePointerComponent<btRigidBody> RigidBodyComponent;
}