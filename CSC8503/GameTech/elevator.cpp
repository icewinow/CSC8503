#include"elevator.h"
#include"../CSC8503Common/StateTransition.h"
#include"../CSC8503Common/StateMachine.h"
#include"../CSC8503Common/State.h"
#include "../CSC8503Common/Ray.h"
#include "../CSC8503Common/CollisionDetection.h"
#include "../../Common/Mouse.h"
#include"../../Common/Window.h"

using namespace NCL;
using namespace CSC8503;

elevator::elevator() {
	counter = 0.0f;
	StateMachine* elevatorState = new StateMachine();

	State* down = new State([&](float dt)->void {
		GetPhysicsObject()->SetLinearVelocity(Vector3(0, -5.0f, 0));
		});

	State* up = new State([&](float dt)->void {
		GetPhysicsObject()->SetLinearVelocity(Vector3(0, 5.0f, 0));
		});

	StateTransition* stateDownToUp = new StateTransition(down, up, [&](void)->bool {
		return GetTransform().GetPosition().y <= 1.1;

	});
	StateTransition* stateUpToDown = new StateTransition(up, down, [&](void)->bool {
		return GetTransform().GetPosition().y >= 15;

		});

	elevatorState->AddState(down);
	elevatorState->AddState(up);

	elevatorState->AddTransition(stateDownToUp);
	elevatorState->AddTransition(stateUpToDown);

	SetStateMachine(elevatorState);

}

elevator::~elevator() {
	delete stateMachine;

}

void elevator::Update(float dt) {
	stateMachine->Update(dt);

}

void elevator::SetStateMachine(StateMachine* stateMachine) {
	this->stateMachine = stateMachine;

}
