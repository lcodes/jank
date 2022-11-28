#include "physics/Physics2D.hpp"

#include <Box2D/Box2D.h>

namespace Physics2D {

static b2World* world;

void init() {
  b2Vec2 gravity{ 0, -9 };
  world = new b2World(gravity);
}

void term() {
  delete world;
}

} // namespace Physics2D
