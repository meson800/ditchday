#pragma once

#include <vector>
#include <map>

#include "GenericParser.h"

class Terrain
{
public:
    static constexpr uint32_t EMPTY  = 0xFFFFFFFF; // white
    static constexpr uint32_t WALL   = 0x000000FF; // black
    static constexpr uint32_t START1 = 0xFF0000FF; // red
    static constexpr uint32_t START2 = 0x0000FFFF; // blue
    static constexpr uint32_t FLAG1  = 0xFFFF00FF; // yellow
    static constexpr uint32_t FLAG2  = 0x00FFFFFF; // cyan
    static constexpr uint32_t MINE   = 0x00FF00FF; // blue

    std::vector<uint32_t> map;
    uint32_t width;
    uint32_t height;
    uint32_t scale;

    uint32_t colorAt(int32_t tx, int32_t ty) const
    {
        if (tx < 0 || tx >= width || ty < 0 || ty >= height) {
            return WALL;
        }
        return map[tx+(height-1-ty)*width];
    }
    
    std::pair<int64_t, int64_t> convertMapCoords(int32_t tx, int32_t ty) const
    {
        return std::pair<int64_t, int64_t>(
            tx * scale + scale/2,
            ty * scale + scale/2
        );
    }
};

class Config
{
public:
    Terrain terrain;

    std::map<uint32_t, std::vector<std::pair<int64_t, int64_t>>> startLocations;
    std::map<uint32_t, std::vector<std::pair<int64_t, int64_t>>> flags;
    std::vector<std::pair<int64_t, int64_t>> mines;

    uint16_t subTurningSpeed;
    uint16_t subAcceleration;
    uint16_t subMaxSpeed;
    uint16_t stealthSpeedLimit;

    uint16_t maxTorpedos;
    uint16_t maxMines;

    uint16_t sonarRange;
    uint16_t passiveSonarNoiseFloor;

    uint16_t torpedoSpread;
    uint16_t torpedoSpeed;
    uint16_t collisionRadius;

    uint16_t torpedoDamage;
    uint16_t mineDamage;
    uint16_t collisionDamage;

    uint16_t mineExclusionRadius;

    uint16_t frameMilliseconds;

    uint16_t stealthCooldown;
    uint16_t respawnCooldown;
};

/**
 * This class takes a ParseResult from the generic parser and
 * turns it into the information (e.g. the map, various configs)
 * included incidential with the game.
 */
class ConfigParser
{
public:
    static Config parseConfig(const ParseResult& parse);
};
