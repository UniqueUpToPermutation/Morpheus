#pragma once

#include "btBulletDynamicsCommon.h"

namespace Morpheus {
	class RigidBodyComponent {
	private:
		btDynamicsWorld* mDynamicsWorld;
		btRigidBody* mRigidBody;
		btMotionState* mMotionState;

	public:
		inline RigidBodyComponent(btDynamicsWorld* dw, btRigidBody* rb, btMotionState* ms) : 
			mDynamicsWorld(dw),
			mRigidBody(rb),
			mMotionState(ms) {
		}

		inline RigidBodyComponent(RigidBodyComponent&& other) {
			mRigidBody = other.mRigidBody;
			mMotionState = other.mMotionState;
			mDynamicsWorld = other.mDynamicsWorld;

			other.mRigidBody = nullptr;
			other.mMotionState = nullptr;
			other.mDynamicsWorld = nullptr;
		}

		inline RigidBodyComponent& operator=(RigidBodyComponent&& other) {
			mRigidBody = other.mRigidBody;
			mMotionState = other.mMotionState;
			mDynamicsWorld = other.mDynamicsWorld;

			other.mRigidBody = nullptr;
			other.mMotionState = nullptr;
			other.mDynamicsWorld = nullptr;

			return *this;
		}

		~RigidBodyComponent() {
			if (mRigidBody) {
				mDynamicsWorld->removeRigidBody(mRigidBody);
				delete mMotionState;
				delete mRigidBody;
			}
		}

		inline btRigidBody* GetRigidBody() {
			return mRigidBody;
		}

		inline btMotionState* GetMotionState() {
			return mMotionState;
		}
	};
}