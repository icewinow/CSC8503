#pragma once
#include"StateGameObject.h"
namespace NCL {
	namespace CSC8503 {
		class SpinWall :public StateGameObject {
		public:
			SpinWall();
			~SpinWall();

			void Update(float dt) override;
			void SetStateMachine(StateMachine* stateMachine);

		protected:

			StateMachine* stateMachine;
			float counter;

		};

	}
}