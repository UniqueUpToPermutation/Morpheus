#include <Engine/Systems/Physics.hpp>

namespace Morpheus {

	void PhysicsSystem::InitPhysics() {
		std::cout << "Initializing physics subsystem..." << std::endl;

		if (!mCollisionConfiguration)
			mCollisionConfiguration = new btDefaultCollisionConfiguration();
		if (!mCollisionDispatcher)
			mCollisionDispatcher = new btCollisionDispatcher(mCollisionConfiguration);
		if (!mBroadphaseInterface)
			mBroadphaseInterface = new btDbvtBroadphase();
		if (!mConstaintSolver)
			mConstaintSolver = new btSequentialImpulseConstraintSolver();
		if (!mWorld)
			mWorld = new btDiscreteDynamicsWorld(mCollisionDispatcher, 
				mBroadphaseInterface,
				mConstaintSolver, 
				mCollisionConfiguration);

		mWorld->setGravity(btVector3(0, -10, 0));
	}

	void PhysicsSystem::KillPhysics() {

		std::cout << "Shutting down physics subsystem..." << std::endl;

		if (mWorld) {
			delete mWorld;
			mWorld = nullptr;
		}

		if (mConstaintSolver) {
			delete mConstaintSolver;
			mConstaintSolver = nullptr;
		}

		if (mBroadphaseInterface) {
			delete mBroadphaseInterface;
			mBroadphaseInterface = nullptr;
		}

		if (mCollisionDispatcher) {
			delete mCollisionDispatcher;
			mCollisionDispatcher = nullptr;
		}

		if (mCollisionConfiguration) {
			delete mCollisionConfiguration;
			mCollisionConfiguration = nullptr;
		}
	}

	void PhysicsSystem::Startup(Scene* scene) {
		InitPhysics();
	}
	
	void PhysicsSystem::Shutdown(Scene* scene) {
		KillPhysics();
	}
}