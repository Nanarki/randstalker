#pragma once

#include "item.hpp"

class World;

class Enemy
{
private :
    uint8_t _id;
    std::string _name;
    uint8_t _health;
    uint8_t _attack;
    uint8_t _defence;
    uint8_t _dropped_golds;
    Item* _dropped_item;
    uint16_t _drop_probability;  ///< 128 ---> 1/128 chance

public:
    Enemy(uint8_t id, const std::string& name, uint8_t health, uint8_t attack, uint8_t defence, 
            uint8_t dropped_golds, Item* dropped_item, uint16_t drop_probability);

    uint8_t id() const { return _id; }
    std::string name() const { return _name; }
    uint8_t health() const { return _health; }
    uint8_t attack() const { return _attack; }
    uint8_t defence() const { return _defence; }
    uint8_t dropped_golds() const { return _dropped_golds; }
    Item* dropped_item() const { return _dropped_item; }
    uint16_t drop_probability() const { return _drop_probability; }

    Json to_json() const;
    static Enemy* from_json(uint8_t id, const Json& json, const World& world);
    void apply_json(const Json& json, const World& world);
};

#define ENEMY_SLIME_1 0x3C
#define ENEMY_SLIME_2 0x3D
#define ENEMY_SLIME_3 0x3E
#define ENEMY_SLIME_4 0x81
#define ENEMY_SLIME_5 0x82
#define ENEMY_SLIME_6 0x83
#define ENEMY_ORC_1 0x04
#define ENEMY_ORC_2 0x05
#define ENEMY_ORC_3 0x06
#define ENEMY_WORM_1 0x07
#define ENEMY_WORM_2 0x08
#define ENEMY_WORM_3 0x09
#define ENEMY_MUSHROOM_1 0x36
#define ENEMY_MUSHROOM_2 0x37
#define ENEMY_MUSHROOM_3 0x38
#define ENEMY_GIANT_1 0x39
#define ENEMY_GIANT_2 0x3A
#define ENEMY_GIANT_3 0x3B
#define ENEMY_REAPER_1 0x88
#define ENEMY_REAPER_2 0x89
#define ENEMY_GOLEM_1 0x92
#define ENEMY_GOLEM_2 0x93
#define ENEMY_GOLEM_3 0x94
#define ENEMY_SPECTRE_1 0x95
#define ENEMY_SPECTRE_2 0x96
#define ENEMY_SPECTRE_3 0x97
#define ENEMY_NINJA_1 0x17
#define ENEMY_NINJA_2 0x18
#define ENEMY_NINJA_3 0x19
#define ENEMY_LIZARD_1 0x1A
#define ENEMY_LIZARD_2 0x1B
#define ENEMY_LIZARD_3 0x1C
#define ENEMY_KNIGHT_1 0x1D
#define ENEMY_KNIGHT_2 0x1E
#define ENEMY_KNIGHT_3 0x1F
#define ENEMY_GHOST_1 0x20
#define ENEMY_GHOST_2 0x21
#define ENEMY_GHOST_3 0x22
#define ENEMY_MUMMY_1 0x23
#define ENEMY_MUMMY_2 0x24
#define ENEMY_MUMMY_3 0x25
#define ENEMY_UNICORN_1 0x26
#define ENEMY_UNICORN_2 0x27
#define ENEMY_UNICORN_3 0x28
#define ENEMY_SKELETON_1 0x29
#define ENEMY_SKELETON_2 0x2A
#define ENEMY_SKELETON_3 0x2B
#define ENEMY_MIMIC_1 0x2C
#define ENEMY_MIMIC_2 0x2D
#define ENEMY_MIMIC_3 0x2E
#define ENEMY_SPINNER_1 0x9D
#define ENEMY_SPINNER_2 0xAA
#define ENEMY_QUAKE_1 0xA7
#define ENEMY_QUAKE_2 0xAB
#define ENEMY_DUKE 0x70
#define ENEMY_MIR 0x7C
#define ENEMY_ZAK 0x85
#define ENEMY_MIRO 0x9F
#define ENEMY_IFRIT 0xA0
#define ENEMY_NOLE 0xA5
#define ENEMY_GOLA 0xA2
