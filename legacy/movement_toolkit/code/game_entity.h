#include "game_vector.cpp"

enum CollisionType
{  
    COLLISION_TYPE_COLLIDE,
    COLLISION_TYPE_INTERSECT,
    COLLISION_TYPE_NONE
};

class GameEntity
{

public:
	GameEntity(vec Position, int height, int width, CollisionType type);
	~GameEntity();
	void onCollide(GameEntity* e);

private:
    vec Position;
    vec Velocity;
    vec Acceleration;
    int height;
    int width;
    Hitbox hitbox;
    CollisionType type;	
};