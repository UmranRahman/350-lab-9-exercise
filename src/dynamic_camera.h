#ifndef DYNAMIC_CAMERA_H
#define DYNAMIC_CAMERA_H

#include <iostream>
#include <limits>

#include "camera.h"

constexpr float INF = std::numeric_limits<float>::infinity();

// AABB in 1D
struct Interval {
    float min;
    float max;

    constexpr bool contains(const Interval& other) const {
        return (min <= other.min) && (max >= other.max);
    }

    constexpr bool contains(float x) const { return (min <= x) && (x <= max); }

    // signedDistance returns a positive smallest distance of x from the interval boundary
    // if it is not contained, else it returns a smallest (in magnitude) negative distance
    // from the interval boundary.
    constexpr float signedDistance(float x) const {
        if (contains(x)) {
            // 0 or negative
            return std::max(x - max, min - x);
        }
        // positive
        return std::max(x - max, min - x);
    }
};

struct DynamicCamera {
    void update(Camera& camera,                           // the camera to update
                const sf::Vector2f& trackedPosWorld,      // world coords of player to track
                const sf::Vector2f& trackedVelocityWorld  // world velocity of player to track
    ) {
        // TODO: (HINTS/outline)
        // collect statistics on the player position and velocity in (normalized, if you like)
        // window coordinates. Compute lookAhead distance in world coords, adjusting for
        // if the player is moving left or right.
        // Now update lookAhead state using an EMA over its previous state and the lookAhead world
        // vector. Then determine if the player is in the dead zone, soft rejection zone, or hard
        // rejection zone. For each, perform EMA on the delta of the camera displacement (ignoring
        // lookahead) appropriately. Next,
        // set the camera center by offsetting the cameraUnadjustedCenter, combining lookAhead.
        // Finally update cameraUnadjustedCenter, but without lookahead.


        // Which way is the player moving? -1 left, 0 still, +1 right.
        float dir = 0.f;
        if (trackedVelocityWorld.x > 0.f) {
            dir = 1.f;
        } else if (trackedVelocityWorld.x < 0.f) {
            dir = -1.f;
        }

        // lookAhead is a window percent; convert it to a world distance (x only).
        float lookAheadWorld = camera.windowPercentToWorldDist({lookAhead, 0.f}).x;

        // Where we'd like the lookahead offset to be, then ease toward it.
        float targetOffset = dir * lookAheadWorld;
        lookaheadState += alpha * (targetOffset - lookaheadState);

        // Width of the view in world units, from the unadjusted camera.
        float viewWidth = camera.windowPercentToWorldDist({1.f, 0.f}).x;
        float unadjustedX = cameraUnadjustedCenter.x;

        // Player's screen position (0..1) relative to the unadjusted camera.
        float px = (trackedPosWorld.x - (unadjustedX - viewWidth * 0.5f)) / viewWidth;

        // Soft zone: ease the camera toward putting the player on the violated green line.
        float overshoot = 0.f;
        if (px < deadZoneX.min) {
            overshoot = px - deadZoneX.min;
        } else if (px > deadZoneX.max) {
            overshoot = px - deadZoneX.max;
        }
        unadjustedX += alpha * overshoot * viewWidth;

        // Hard zone: re-measure, then snap so the player is at most ON the red line.
        px = (trackedPosWorld.x - (unadjustedX - viewWidth * 0.5f)) / viewWidth;
        if (px < softZoneX.min) {
            unadjustedX += (px - softZoneX.min) * viewWidth;
        } else if (px > softZoneX.max) {
            unadjustedX += (px - softZoneX.max) * viewWidth;
        }

        // Real camera = zone-only camera + lookahead offset.
        camera.setCameraCenter({unadjustedX + lookaheadState, cameraUnadjustedCenter.y});

        // Save the lookahead-free center for next frame.
        cameraUnadjustedCenter.x = unadjustedX;
    }

    float alpha;  // alpha to use for damping of camera position, *as well as* for smoothing of
                  // lookahead offset.
    Interval deadZoneX;  // percentage interval along x-axis to use for dead zone in [0, 1]
    // percentage interval along x-axis to use for soft rejection zone in [0, 1]
    Interval softZoneX = {-INF, INF};
    // (everything beyond softZone is considered hard zone).
    float lookAhead;  // screen-percentage lookahead
    mutable sf::Vector2f
        cameraUnadjustedCenter;      // the camera world center from last frame, ignoring lookahead
    mutable float lookaheadState{};  // the EMA of the accumulated lookahead offset in world coords
};

#endif // DYNAMIC_CAMERA_H
