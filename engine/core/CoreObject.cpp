#include "core/CoreObject.hpp"

void Object::load(std::istream& i) {
}

void Object::save(std::ostream& o) const {
  o << id;
}
