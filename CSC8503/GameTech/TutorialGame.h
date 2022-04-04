#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include"StateGameObject.h"
#include "../CSC8503Common/NavigationGrid.h"
#include"Enemy.h"

namespace NCL {
	namespace CSC8503 {
		enum class MenuState {
			Start,
			Running,
			Pause,
			End,
		};
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);
			void menu(float dt);

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			void InitGameExamples();

			void InitpartA();
			void InitpartB();
			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitDefaultFloor();
			void BridgeConstraintTest();
	
			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();
			void eatCoin();
			void fail();
			void win();
			void checkEnemy();

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(string name,const Vector3 & position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(string name,const Vector3& position, Vector3 dimensions, float inverseMass, float elasticity);
			StateGameObject* AddElevatorToWorld(const Vector3& position, Vector3 dimensions, float inverseMass);
			GameObject* AddCoinToWorld(const Vector3& position, Vector3 dimensions, float inverseMass,Vector3 rotation);
			StateGameObject* AddSpinWallToWorld(const Vector3& position, Vector3 dimensions);
			
			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f);

			GameObject* AddPlayerToWorld(string name,const Vector3& position);
			Enemy* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;
			MenuState           state;
			NavigationGrid*     Grid;

			StateGameObject* AddStateObjectToWorld(const Vector3& position);

			StateGameObject* testStateObject;
			std::set<CollisionDetection::CollisionInfo> allCollisions;

			bool useGravity;
			bool inSelectionMode;
			bool wined;
			bool failed;
			bool isPartA;

			float		forceMagnitude;
			int score;

			GameObject* selectionObject = nullptr;
			Enemy* enemy;

			OGLMesh*	capsuleMesh = nullptr;
			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Meshes
			OGLMesh*	charMeshA	= nullptr;
			OGLMesh*	charMeshB	= nullptr;
			OGLMesh*	enemyMesh	= nullptr;
			OGLMesh*	bonusMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			//EnemyAI
			void EnemyAI(Enemy* enemy);

		};
	}
}

