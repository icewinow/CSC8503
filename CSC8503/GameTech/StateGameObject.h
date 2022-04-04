#pragma once
#include"..\CSC8503Common\GameObject.h"
namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class StateGameObject :public GameObject {
		public:
			StateGameObject();
			~StateGameObject();

			void Update(float dt) override;
			void SetStateMachine(StateMachine* stateMachine);

		protected:

			StateMachine* stateMachine;
			float counter;

		};

	}
}