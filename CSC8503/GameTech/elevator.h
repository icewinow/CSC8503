#pragma once
#include"StateGameObject.h"
namespace NCL {
	namespace CSC8503 {
		class elevator :public StateGameObject {
		public:
			elevator();
			~elevator();

			void Update(float dt) override;
			void SetStateMachine(StateMachine* stateMachine);

		protected:

			StateMachine* stateMachine;
			float counter;

		};

	}
}