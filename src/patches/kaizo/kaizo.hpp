#pragma once

#include "../../tools/megadrive/rom.hpp"
#include "../../tools/megadrive/code.hpp"

#include "../../world.hpp"

#include "../../model/map.hpp"
#include "../../model/map_palette.hpp"
#include "../../model/entity.hpp"
#include "../../model/entity_type.hpp"

#include "../../constants/entity_type_codes.hpp"
#include "../../constants/map_codes.hpp"
#include "../../constants/item_codes.hpp"
#include "../../constants/offsets.hpp"

////// EDIT CHAPTER FUNCTIONS ///////////////////////////////////////////////////////////////////

void patch_engine_for_kaizo(World& world, md::ROM& rom);
void edit_safety_pass_arc(World& world, md::ROM& rom);
void edit_mercator(World& world, md::ROM& rom);
void edit_mercator_castle(World& world, md::ROM& rom);

////// ENTITY TYPE ALIASES ///////////////////////////////////////////////////////////////////

constexpr uint8_t POWER_GLOVE_BLOCKER_TYPE = ENTITY_LARGE_BLUE_BOULDER;
constexpr uint8_t QUARTZ_SWORD_BLOCKER_TYPE = ENTITY_GOLD_GOLEM_STATUE;

////// USEFUL BEHAVIOR IDS ///////////////////////////////////////////////////////////////////

constexpr uint16_t BEHAVIOR_DISAPPEAR_ON_ALL_ENEMIES_BEATEN = 106;
constexpr uint16_t BEHAVIOR_ALLOW_APPEAR_AFTER_PLAYER_MOVED_AWAY = 785;
constexpr uint16_t BEHAVIOR_BUTTON_SET_PERSISTENCE_FLAG = 544;
constexpr uint16_t BEHAVIOR_INVISIBLE_REVEALABLE_BY_GOLAS_EYE = 531;

////// FLAGS /////////////////////////////////////////////////////////////////// 

static const Flag FLAG_ALWAYS_TRUE                      = Flag(0x00, 0);
static const Flag FLAG_HELGA_TALKED_ABOUT_WORM          = Flag(0x04, 4);
static const Flag FLAG_KAYLA_BATHROOM_BUTTON_PRESSED    = Flag(0x06, 0);
static const Flag FLAG_DEXTER_ROOM_BUTTON_PRESSED       = Flag(0x06, 1);
static const Flag FLAG_ZAK_ROOM_BUTTON_PRESSED          = Flag(0x06, 2);

static const Flag FLAG_SAFETY_PASS_OWNED                = Flag(0x59, 5);