#include"SpinWall.h"
#include"../CSC8503Common/StateTransition.h"
#include"../CSC8503Common/StateMachine.h"
#include"../CSC8503Common/State.h"
#include "../CSC8503Common/Ray.h"
#include "../CSC8503Common/CollisionDetection.h"

using namespace NCL;
using namespace CSC8503;

SpinWall::SpinWall() {
	counter = 0.0f;
	StateMachine* SpinWallState = new StateMachine();
	
	State* spin = new State([&](float dt)->void {
		Quaternion q = this->GetTransform().GetOrientation();
		this->GetTransform().SetOrientation(q + Quaternion(Vector3(0, 1, 0) * dt * 0.5f, 0.0f) * q);
	});

	SpinWallState->AddState(spin);

	SetStateMachine(SpinWallState);

}

SpinWall::~SpinWall() {
	delete stateMachine;

}

void SpinWall::Update(float dt) {
	stateMachine->Update(dt);

}

void SpinWall::SetStateMachine(StateMachine* stateMachine) {
	this->stateMachine = stateMachine;

}