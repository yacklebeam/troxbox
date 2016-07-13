#include "game_entity.h"

GameEntity::GameEntity(vec P, int H, int W, CollisionType T)
{
	Position = P;
	height = H;
	width = W;
	type = T;
	Velocity = MakeVec(0.0f, 0.0f);
	Acceleration = MakeVec(0.0f, 0.0f);
	hitbox;
}

GameEntity::~GameEntity()
{

}

void GameEntity::onCollide(GameEntity *e)
{

}