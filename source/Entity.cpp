#include "Entity.hpp"

// Entity is fully defined in its header: its members are public data
// (RigidBody2D, Collision2D, active, id), the destructor is defaulted and
// bounds() is inline. This translation unit exists so the class has its own
// .cpp like every other one, and so the header is checked to compile on its
// own (it must include everything it uses).
