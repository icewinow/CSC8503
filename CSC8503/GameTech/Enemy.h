#pragma once
#include"..\CSC8503Common\GameObject.h"
#include"../CSC8503Common/BehaviourNode.h"
#include"../CSC8503Common/BehaviourAction.h"
#include"../CSC8503Common/BehaviourSelector.h"
#include"../CSC8503Common/BehaviourSequence.h"
#include"../CSC8503Common/GameWorld.h"
#include"../CSC8503Common/CollisionDetection.h"
#include <set>


namespace NCL {
	namespace CSC8503 {
		class Enemy :public GameObject {
		public:
			Enemy();
			~Enemy();

			void Update(float dt) override;
			void SetBehaviourTree(BehaviourNodeWithChildren* BehaviourTree);
			void findPath(GameObject* target);
			vector<Vector3> GetPathNodes() {
				return pathNodes;
			}
			void fllowPath();
			void SetHaveBounes(bool a) {
				haveBounes = a;
			}
			void SetgetBounes(bool a) {
				getBounes = a;
			}
			void Setgetplayer(bool a) {
				getplayer = a;
			}
			void SetTarget(GameObject* g) {
				target = g;
			}
			bool GetState() {
				return onBounes;
			}
			void DisplayPath();

		protected:
			BehaviourNodeWithChildren* BehaviourTree;
			vector<Vector3> pathNodes;
			float counter;
			GameObject* target;
			Vector3 pastdirction;
			bool haveBounes;
			bool getBounes;
			bool getplayer;
			bool onBounes;


		};

	}
}