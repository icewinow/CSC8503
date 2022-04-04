#include"Enemy.h"
#include"../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include <algorithm>

using namespace NCL;
using namespace CSC8503;

Enemy::Enemy() {
	name = "enemy";
	counter = 0.0f;
	getBounes = false;
	haveBounes = false;
	getplayer = false;

	BehaviourAction* findBonuses2 = new BehaviourAction("find bonuses", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "finding bonuses by pathfind\n";
			return Ongoing;

		}
		else if (state == Ongoing) {
			onBounes = true;
			if (getBounes) {
				return Failure;
			}
			if (haveBounes) {
				findPath(target);
				fllowPath();
			}
			else {
				return Failure;
			}
		}
		return state;

	});

	BehaviourAction* findPlayer2 = new BehaviourAction("find Player", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "finding player by pathfind\n";
			return Ongoing;

		}
		else if (state == Ongoing) {
			onBounes = false;
			if (getplayer) {
				return Success;
			}
			
			findPath(target);
            fllowPath();

		}
		return state;

	});

	BehaviourSelector* findSelection = new BehaviourSelector("find selection");
	findSelection->AddChild(findBonuses2);
	findSelection->AddChild(findPlayer2);
	
	
	BehaviourTree = findSelection;
	BehaviourTree->Reset();

}

Enemy::~Enemy() {
	delete BehaviourTree;
	delete target;

}
void Enemy::findPath(GameObject* target) {
	pathNodes.clear();

	NavigationGrid grid("TestGrid1.txt");

	NavigationPath outPath;

	Vector3 startPos = this->GetTransform().GetPosition();
	Vector3 endPos = target->GetTransform().GetPosition();

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		pathNodes.push_back(pos);
	}

}

void Enemy::fllowPath() {
	if (pathNodes.size() <= 1) {
		this->GetPhysicsObject()->AddForce(pastdirction.Normalised() * 20.0f);
	}
	else {
		Vector3 nextNode = pathNodes[1];

		Vector3 dirction = nextNode - this->GetTransform().GetPosition();
		this->GetPhysicsObject()->AddForce(dirction.Normalised() * 10.0f);
		pastdirction = dirction;
		DisplayPath();

	}

}

void Enemy::Update(float dt) {
	BehaviourTree->Execute(dt);

}

void Enemy::DisplayPath() {
	for (int i = 1; i < pathNodes.size(); ++i) {
		Vector3 a = pathNodes[i - 1];
		Vector3 b = pathNodes[i];
		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}
	
}

void Enemy::SetBehaviourTree(BehaviourNodeWithChildren* BT) {
	this->BehaviourTree = BT;

}