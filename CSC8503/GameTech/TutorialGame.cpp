#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"

#include "../CSC8503Common/PositionConstraint.h"

#include"../CSC8503Common/State.h"
#include"../CSC8503Common/StateTransition.h"
#include"../CSC8503Common/StateMachine.h"

#include"../CSC8503Common/BehaviourNode.h"
#include"../CSC8503Common/BehaviourAction.h"
#include"../CSC8503Common/BehaviourSelector.h"
#include"../CSC8503Common/BehaviourSequence.h"

#include"elevator.h"
#include"SpinWall.h"
#include <algorithm>

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;

	score = 0;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("Male1.msh"	 , &charMeshA);
	loadFunc("courier.msh"	 , &charMeshB);
	loadFunc("security.msh"	 , &enemyMesh);
	loadFunc("coin.msh"		 , &bonusMesh);
	loadFunc("capsule.msh"	 , &capsuleMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::menu(float dt) {
	switch (state)
	{
	case MenuState::Start:
		Debug::Print("Push Player to the end!", Vector2(20, 30), Vector4(1, 0, 0, 1));
		Debug::Print("(1) Part A", Vector2(20, 40), Debug::BLUE);
		Debug::Print("(2) Part B)", Vector2(20, 50), Debug::BLUE);
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM1)) {
			isPartA = true;
			world->ClearAndErase();
			InitialiseAssets();
			InitCamera();
			InitWorld();
			state = MenuState::Running;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NUM2)) {
			isPartA = false;
			world->ClearAndErase();
			InitialiseAssets();
			InitCamera();
			InitWorld();
			state = MenuState::Running;
		}

		break;

	case MenuState::Running:
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P)) {
			state = MenuState::Pause;
		}
		if (failed) {
			state = MenuState::End;
		}
		if (wined) {
			state = MenuState::End;
		}

		UpdateGame(dt);
		break;

	case MenuState::Pause:
		Debug::Print("Press (P) to Resume", Vector2(20, 40), Debug::BLUE);
		Debug::Print("Press (R) to Restart", Vector2(20, 50), Debug::BLUE);
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P)) {
			state = MenuState::Running;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::R)) {
			state = MenuState::Start;
		}
		break;

	case MenuState::End:
		if (wined) {
			Debug::Print("You Win!", Vector2(20, 30), Debug::BLUE);
		}
		else if(failed){
			Debug::Print("You Lose!", Vector2(20, 30), Debug::BLUE);
		}
		Debug::Print("Your Score:" + std::to_string(score), Vector2(20, 40), Debug::BLUE);
		Debug::Print("Press (R) to Restart", Vector2(20, 50), Debug::BLUE);
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::R)) {
			state = MenuState::Start;
		}
		break;
	}
	Debug::FlushRenderables(dt);
	renderer->Render();

}

void TutorialGame::UpdateGame(float dt) {
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	UpdateKeys();

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95));
	}

	SelectObject();
	MoveSelectedObject();
	physics->Update(dt);
	allCollisions = physics->GetCollisionlist();

	checkEnemy();
	eatCoin();
	fail();
	win();

	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0,1,0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);

		//Debug::DrawAxisLines(lockedObject->GetTransform().GetMatrix(), 2.0f);
	}

	world->UpdateWorld(dt);
	renderer->Update(dt);

	//Debug::FlushRenderables(dt);
	//renderer->Render();
}

void TutorialGame::eatCoin() {
	bool eated = false;
	for (std::set<CollisionDetection::CollisionInfo>::iterator i = allCollisions.begin(); i != allCollisions.end(); ) {
		if (i->a->GetName() == "leader" && i->b->GetName() == "coin") {
			eated = true;
			world->RemoveGameObject(i->b, false);
			break;
		}
		else if (i->b->GetName() == "leader" && i->a->GetName() == "coin") {
			eated = true;
			world->RemoveGameObject(i->a, false);
			break;
		}
		++i;

	}
	if (eated) {
		score++ ;
	}

}

void TutorialGame::fail() {
	bool failed = false;
	for (std::set<CollisionDetection::CollisionInfo>::iterator i = allCollisions.begin(); i != allCollisions.end(); ) {
		if (i->a->GetName() == "leader" && i->b->GetName() == "floor") {
			failed = true;
			world->RemoveGameObject(i->b, false);
			break;
		}
		else if (i->b->GetName() == "leader" && i->a->GetName() == "floor") {
			failed = true;
			world->RemoveGameObject(i->a, false);
			break;
		}
		if (i->a->GetName() == "enemy" && i->b->GetName() == "player") {
			failed = true;

		}
		else if (i->b->GetName() == "enemy" && i->a->GetName() == "player") {
			failed = true;

		}
		++i;

	}
	GameObjectIterator begin;
	GameObjectIterator end;
	world->GetObjectIterators(begin, end);

	for (GameObjectIterator i = begin; i != end;) {
		if ((*i)->GetName() == "leader" && (*i)->GetTransform().GetPosition().y > 30) {
			failed = true;
			break;
		}
		else if ((*i)->GetName() == "leader" && (*i)->GetTransform().GetPosition().y < 0) {
			failed = true;
			break;
		}

		++i;
	}
	this->failed = failed;

}
void TutorialGame::win() {
	bool wined = false;
	for (std::set<CollisionDetection::CollisionInfo>::iterator i = allCollisions.begin(); i != allCollisions.end(); ) {
		if (i->a->GetName() == "leader" && i->b->GetName() == "destination") {
			wined = true;
			world->RemoveGameObject(i->b, false);
			break;
		}
		else if (i->b->GetName() == "leader" && i->a->GetName() == "destination") {
			wined = true;
			world->RemoveGameObject(i->a, false);
			break;
		}
		++i;

	}
	this->wined = wined;
}

void TutorialGame::checkEnemy() {

	for (std::set<CollisionDetection::CollisionInfo>::iterator i = allCollisions.begin(); i != allCollisions.end(); ) {
		if (i->a->GetName() == "enemy" && i->b->GetName() == "player") {
			enemy->Setgetplayer(true);
			
		}
		else if (i->b->GetName() == "enemy" && i->a->GetName() == "player") {
			enemy->Setgetplayer(true);

		}
		if (i->a->GetName() == "enemy" && i->b->GetName() == "bonuses") {
			enemy->SetgetBounes(true);
			world->RemoveGameObject(i->b, false);
		}
		else if (i->b->GetName() == "enemy" && i->a->GetName() == "bonuses") {
			enemy->SetgetBounes(true);
			world->RemoveGameObject(i->a, false);
		}
		++i;


	}

	GameObjectIterator begin;
	GameObjectIterator end;
	world->GetObjectIterators(begin, end);
	for (GameObjectIterator i = begin; i != end;) {
		if ((*i)->GetName() == "bonuses") {
			enemy->SetHaveBounes(true);
			if (enemy->GetState()) {
				enemy->SetTarget(*i);
			}

		}
		else if ((*i)->GetName() == "player" && !enemy->GetState()) {
			enemy->SetTarget(*i);
		}
		++i;
	}


}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		lockedObject	= nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward  = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);
	Vector3 charForward2 = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);

	float force = 100.0f;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(-1,0,0) * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		Vector3 worldPos = selectionObject->GetTransform().GetPosition();
		lockedObject->GetPhysicsObject()->AddForce(Vector3(1, 0, 0) * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -1) * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 1) * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::SHIFT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0,-1,0) * force);
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::SHIFT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0, 1, 0) * force);
	}
}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}

}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	score = 0;
	world->ClearAndErase();
	physics->Clear();
	if (isPartA) {
		InitpartA();
	}
	else {
		InitpartB();
	}

}


void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(8, 4, 8);

	float invCubeMass = 5;
	int numLinks = 3;
	float maxDistance = 30;
	float cubeDistance = 20;

	Vector3 startPos = Vector3(100, -6.5, 7.5);
	GameObject* start = AddCubeToWorld("bridge",startPos + Vector3(0, 0, 0), cubeSize, 0,0.8f);
	GameObject* end = AddCubeToWorld("bridge",startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0,0.8f);
	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld("bridge",startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass,0.8f);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;

	}
	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);

}

/*

A single function to add a large immoveable cube to the bottom of our world

*/

GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject("floor");

	Vector3 floorSize	= Vector3(120, 2, 120);
	AABBVolume* volume	= new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume(),0.8f));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(string name,const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject(name);

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume(), 0.8f));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject();

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius* 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume(), 0.8f));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;

}

GameObject* TutorialGame::AddCubeToWorld(string name,const Vector3& position, Vector3 dimensions, float inverseMass,float elasticity) {
	GameObject* cube = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume(), elasticity));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

StateGameObject* TutorialGame::AddElevatorToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	elevator* el = new elevator();

	AABBVolume* volume = new AABBVolume(dimensions);

	el->SetBoundingVolume((CollisionVolume*)volume);

	el->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	el->SetRenderObject(new RenderObject(&el->GetTransform(), cubeMesh, basicTex, basicShader));
	el->SetPhysicsObject(new PhysicsObject(&el->GetTransform(), el->GetBoundingVolume(),0.8f));

	el->GetPhysicsObject()->SetInverseMass(inverseMass);
	el->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(el);

	return el;
}

GameObject* TutorialGame::AddCoinToWorld(const Vector3& position, Vector3 dimensions, float inverseMass,Vector3 rotation) {
	GameObject* coin = new GameObject("coin");

	AABBVolume* volume = new AABBVolume(dimensions);

	coin->SetBoundingVolume((CollisionVolume*)volume);

	coin->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);
	coin->GetTransform().SetOrientation(Quaternion(rotation, 0.0f));

	coin->SetRenderObject(new RenderObject(&coin->GetTransform(), cubeMesh, basicTex, basicShader));
	coin->SetPhysicsObject(new PhysicsObject(&coin->GetTransform(), coin->GetBoundingVolume(),0.8f));

	coin->GetPhysicsObject()->SetInverseMass(inverseMass);
	coin->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(coin);

	return coin;
}

StateGameObject* TutorialGame::AddSpinWallToWorld(const Vector3& position, Vector3 dimensions) {
	SpinWall* spinwall = new SpinWall();

	AABBVolume* volume = new AABBVolume(dimensions);

	spinwall->SetBoundingVolume((CollisionVolume*)volume);

	spinwall->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	spinwall->SetRenderObject(new RenderObject(&spinwall->GetTransform(), cubeMesh, basicTex, basicShader));
	spinwall->SetPhysicsObject(new PhysicsObject(&spinwall->GetTransform(), spinwall->GetBoundingVolume(),0.8f));

	spinwall->GetPhysicsObject()->SetInverseMass(0);
	spinwall->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(spinwall);

	return spinwall;
}



/*
void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}*/

void TutorialGame::InitpartA() {
	//left long wall
	AddCubeToWorld("wall",Vector3(-90, 12, 0), Vector3(1, 10, 90), 0.0f,0.4f);
	//Left top bot
	AddCubeToWorld("wall", Vector3(-60, 12, 90), Vector3(29, 10, 1), 0.0f, 0.4f);
	AddCubeToWorld("wall", Vector3(-60, 12, -90), Vector3(29, 10, 1), 0.0f, 0.4f);
	//left short wall
	AddCubeToWorld("wall", Vector3(-30, 12, -60), Vector3(1, 10, 30), 0.0f, 0.4f);
	AddCubeToWorld("wall", Vector3(-30, 12, 60), Vector3(1, 10, 30), 0.0f, 0.4f);
	//mid wall
	AddCubeToWorld("wall", Vector3(0, 12, -30), Vector3(29, 10, 1), 0.0f, 0.4f);
	AddCubeToWorld("wall", Vector3(0, 12, 30), Vector3(29, 10, 1), 0.0f, 0.4f);
	//right short wall
	AddCubeToWorld("wall", Vector3(30, 12, -60), Vector3(1, 10, 30), 0.0f, 0.4f);
	AddCubeToWorld("wall", Vector3(30, 12, 60), Vector3(1, 10, 30), 0.0f, 0.4f);
	//right top bot
	AddCubeToWorld("wall", Vector3(60, 12, 90), Vector3(29, 10, 1), 0.0f, 0.4f);
	AddCubeToWorld("wall", Vector3(60, 12, -90), Vector3(29, 10, 1), 0.0f, 0.4f);
	//right long wall
	AddCubeToWorld("wall", Vector3(90, 12, 0), Vector3(1, 10, 90), 0.0f, 0.4f);

	//sphere
	AddSphereToWorld("leader",Vector3(-60, 7, 60), 2, 1.0f);

	//ground
	AddCubeToWorld("wall", Vector3(-60, 3, 0), Vector3(29, 1, 89), 0.0f, 0.4f);
	AddCubeToWorld("wall", Vector3(60, 10, 0), Vector3(29, 1, 89), 0.0f, 0.4f);

	//coin
	AddCoinToWorld(Vector3(-60, 12, -60), Vector3(5, 5, 5), 0.0f,Vector3(0.5,0.5,0.5));

	//elevator
	AddElevatorToWorld(Vector3(0, 1, 0), Vector3(30, 1, 29), 0.0f);

	//spinwall
	AddSpinWallToWorld(Vector3(-60, 12, -30), Vector3(16, 8, 1));

	//destination
	AddCubeToWorld("destination", Vector3(60, 19, 60), Vector3(29, 6, 1), 0.0f, 0.4f);

	//AddEnemyToWorld(Vector3(80, 30, 10));

	//floor
	AddFloorToWorld(Vector3(0, -2, 0));
	
}

void TutorialGame::InitpartB() {
	Grid = new NavigationGrid("TestGrid1.txt");
	GridNode* nodes = Grid->GetNodes();
	int gridWidth = Grid->GetgridWidth();
	int gridHeight = Grid->GetgridHeight();
	for (int y = 0; y < gridHeight; ++y) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode& n = nodes[(gridWidth * y) + x];
			if (n.gridType == GridType::Block) {
				AddCubeToWorld("wall",n.position, Vector3(5, 2.5, 5), 0.0f,0.8f);
			}
		}
	}
	AddCubeToWorld("ground",Vector3(45, -3.5, 45),Vector3(45,1,45),0.0f,0.8f);
	BridgeConstraintTest();
	AddPlayerToWorld("player", Vector3(12.5, 5, 12.5));
	AddEnemyToWorld(Vector3(76.5, 5, 76.5));
	AddSphereToWorld("bonuses",Vector3(20, 10, 50), 2, 1.0f);


}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -2, 0));
}
/*
void TutorialGame::InitGameExamples() {
	AddPlayerToWorld(Vector3(0, 5, 0));
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
}
*/
GameObject* TutorialGame::AddPlayerToWorld(string name,const Vector3 & position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject(name);

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	if (rand() % 2) {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshA, nullptr, basicShader));
	}
	else {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshB, nullptr, basicShader));
	}
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume(),0.4f));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();
	world->AddGameObject(character);

	//lockedObject = character;

	return character;
}

/*Enemy* TutorialGame::AddEToWorld(const Vector3& position) {
	Enemy* spinwall = new Enemy();

	AABBVolume* volume = new AABBVolume(Vector3(1, 1, 1));

	spinwall->SetBoundingVolume((CollisionVolume*)volume);

	spinwall->GetTransform()
		.SetPosition(position)
		.SetScale(Vector3(1, 1, 1) * 2);

	spinwall->SetRenderObject(new RenderObject(&spinwall->GetTransform(), cubeMesh, basicTex, basicShader));
	spinwall->SetPhysicsObject(new PhysicsObject(&spinwall->GetTransform(), spinwall->GetBoundingVolume(), 0.8f));

	spinwall->GetPhysicsObject()->SetInverseMass(0.5);
	spinwall->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(spinwall);

	enemy = spinwall;

	return spinwall;
}*/

Enemy* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	Enemy* character = new Enemy();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetPosition(position)
		.SetScale(Vector3(meshSize, meshSize, meshSize));

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume(),0.8f));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);
	enemy = character;

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject("bonuses");

	SphereVolume* volume = new SphereVolume(2.0f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(1, 1, 1))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume(),0.8f));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) {
	StateGameObject* apple = new StateGameObject();

	SphereVolume* volume = new SphereVolume(0.25f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume(),0.8f));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;

}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
				lockedObject	= nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				Debug::Print("foce: " + std::to_string((int)selectionObject->GetPhysicsObject()->GetForce().x) + ", " +
					std::to_string((int)selectionObject->GetPhysicsObject()->GetForce().y) + ", " +
					std::to_string((int)selectionObject->GetPhysicsObject()->GetForce().z) + ", "
					, Vector2(50, 50), Debug::BLUE);
				Debug::Print("position: " + std::to_string((int)selectionObject->GetTransform().GetPosition().x) + ", " +
					std::to_string((int)selectionObject->GetTransform().GetPosition().y) + ", " +
					std::to_string((int)selectionObject->GetTransform().GetPosition().z) + ", "
					, Vector2(50, 30), Debug::BLUE);

				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
	}

	if (lockedObject) {
		renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
	}

	else if(selectionObject){
		renderer->DrawString("Press L to lock selected object object!", Vector2(5, 80));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}

	}

	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void TutorialGame::MoveSelectedObject() {
	renderer->DrawString("Click Force:" + std::to_string(forceMagnitude), Vector2(10, 20));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;

	}

	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude,closestCollision.collidedAt);
			}
		}

	}

}


//enemy ai
/*
void TutorialGame::EnemyAI(Enemy* enemy) {
	BehaviourAction* findBonuses1 = new BehaviourAction("find bonuses by pathfind", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "finding bonuses by pathfind\n";
			return Ongoing;

		}
		else if (state == Ongoing) {
			bool geted = false;
			GameObjectIterator begin;
			GameObjectIterator end;
			world->GetObjectIterators(begin, end);

			for (GameObjectIterator i = begin; i != end;) {
				if ((*i)->GetName() == "enemy") {
					for (GameObjectIterator j = begin; j != end;) {
						if ((*j)->GetName() == "bonuses") {
							((*i)->GetTransform().GetPosition() - (*j)->GetTransform().GetPosition()).Length() < 50;
							geted = true;
						}
					}

				}

				++i;
			}
			if (geted) {
				return Success;
			}
		}

		return state;

		});
	BehaviourAction* findBonuses2 = new BehaviourAction("find bonuses by pathfind", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "finding bonuses by pathfind\n";
			return Ongoing;

		}
		else if (state == Ongoing) {
			bool geted = false;
			for (std::set<CollisionDetection::CollisionInfo>::iterator i = allCollisions.begin(); i != allCollisions.end(); ) {
				if (i->a->GetName() == "enemy" && i->b->GetName() == "bonuses") {
					geted = true;
					world->RemoveGameObject(i->b, false);
					break;
				}
				else if (i->b->GetName() == "enemy" && i->a->GetName() == "bonuses") {
					geted = true;
					world->RemoveGameObject(i->a, false);
					break;
				}
				++i;

			}
			if (geted) {
				return Success;
			}

		}

		return state;

		});
	BehaviourAction* findPlayer1 = new BehaviourAction("find Player by pathfind", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "finding player by pathfind\n";
			return Ongoing;

		}
		else if (state == Ongoing) {
			bool geted = false;
			for (std::set<CollisionDetection::CollisionInfo>::iterator i = allCollisions.begin(); i != allCollisions.end(); ) {
				if (i->a->GetName() == "enemy" && i->b->GetName() == "bonuses") {
					geted = true;
					world->RemoveGameObject(i->b, false);
					break;
				}
				else if (i->b->GetName() == "enemy" && i->a->GetName() == "bonuses") {
					geted = true;
					world->RemoveGameObject(i->a, false);
					break;
				}
				++i;

			}
			if (geted) {
				return Success;
			}
		}

		return state;

		});
	BehaviourAction* findPlayer2 = new BehaviourAction("find Player by pathfind", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "finding player by pathfind\n";
			return Ongoing;

		}
		else if (state == Ongoing) {
			bool geted = false;
			for (std::set<CollisionDetection::CollisionInfo>::iterator i = allCollisions.begin(); i != allCollisions.end(); ) {
				if (i->a->GetName() == "enemy" && i->b->GetName() == "player") {
					geted = true;
					break;
				}
				else if (i->b->GetName() == "enemy" && i->a->GetName() == "player") {
					geted = true;
					break;
				}
				++i;

			}
			if (geted) {

				return Success;
			}
			



		}

		return state;

		});


	BehaviourSelector* findSelection = new BehaviourSelector("find selection");
	}
	*/
