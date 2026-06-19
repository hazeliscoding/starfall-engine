#pragma once

#include "engine/math/vec2.hpp"

namespace Engine::Scene {

class Tilemap;

// Per-axis-independent AABB sweep against a tilemap's solid tiles.
//
// Given a current position, a desired displacement, and an AABB size,
// returns the displacement actually applied (may be less than the input
// on either axis). Tests X-axis then Y-axis independently; if a single
// axis's move would penetrate a solid tile, that axis is rejected while
// the other still applies — producing the standard "slide along walls"
// feel.
//
// Limitation: assumes the move is small enough that the swept AABB
// doesn't tunnel through a thin wall in a single frame. At walking
// speeds (60 px/s) this is never violated; teleports / dashes should
// handle their own collision.
Engine::Math::Vec2 SweepMove(Engine::Math::Vec2 currentPos,
                             Engine::Math::Vec2 displacement,
                             Engine::Math::Vec2 aabbSize,
                             const Tilemap&     tilemap);

}  // namespace Engine::Scene
