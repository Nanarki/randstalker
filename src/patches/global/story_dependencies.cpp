#include <md_tools.hpp>

#include "../../world_model/world.hpp"
#include "../../world_model/map.hpp"
#include "../../world_model/entity.hpp"
#include "../../world_model/entity_type.hpp"
#include "../../constants/map_codes.hpp"

void make_massan_elder_reward_not_story_dependant(md::ROM& rom)
{
    // This item source writes its contents at this specific address, as specified
    // by the model files
    uint8_t reward_item_id = rom.get_byte(0x25F9E);

    md::Code elder_dialogue;
    elder_dialogue.btst(5, addr_(0xFF1002));
    elder_dialogue.bne(3);
        elder_dialogue.trap(0x01, { 0x00, 0x05 });
        elder_dialogue.rts();
        elder_dialogue.add_bytes({ 0xE0, 0xFD });
    elder_dialogue.btst(2, addr_(0xFF1004));
    elder_dialogue.bne(3);
        elder_dialogue.trap(0x01, { 0x00, 0x05 });
        elder_dialogue.rts();
        elder_dialogue.add_bytes({ 0x14, 0x22, 
                                   0x80, 0xFE, 
                                   0x80, 0xFF, 
                                   0x81, 0x00, 
                                   0x00, reward_item_id, 
                                   0x17, 0xE8, 
                                   0x18, 0x00, 
                                   0xE1, 0x01 });
    elder_dialogue.trap(0x01, { 0x00, 0x05 });
    elder_dialogue.rts();
    elder_dialogue.add_bytes({ 0xE1, 0x01 });

    uint32_t addr = rom.inject_code(elder_dialogue);

    rom.set_code(0x25F98, md::Code().jmp(addr).rts());

    rom.mark_empty_chunk(0x25FA0, 0x25FB1);
}

void make_lumberjack_reward_not_story_dependant(md::ROM& rom)
{
    rom.set_word(0x272C4, 0xEAFA); // Change from HandleProgressDependentDialogue to SetFlagBitOnTalking
    rom.set_word(0x272C6, 0x0124); // bit 4 of 1024
    rom.set_word(0x272C8, 0x0EF7); // Flag clear: Einstein whistle gift cutscene
    rom.set_word(0x272CA, 0x15D3); // Flag set: Thanks
    rom.set_word(0x272CC, 0x4E75); // rts

    // Shorten the cutscene by removing all references to the whistle (which we most likely won't get here)
    rom.set_word(0x28888, 0x0);
    rom.set_word(0x2888A, 0x0);
    rom.set_word(0x28890, 0x0);
    rom.set_word(0x28890, 0x0);
    rom.set_word(0x28892, 0x0);
    rom.set_word(0x28894, 0x0);

    // Remove Nigel turning around during the cutscene
    rom.set_code(0x14102, md::Code().nop(12));
}

void change_falling_ribbon_position(md::ROM& rom)
{
    // Change falling item position from 1F to 20 to ensure it is taken by Nigel whatever its original position is
    rom.set_byte(0x09C59E, 0x20);
}

void make_tibor_always_open(md::ROM& rom)
{
    // Make Tibor open without saving the mayor
    rom.set_byte(0x501D, 0x10);
}

void make_gumi_boulder_push_not_story_dependant(World& world)
{
    // Turn the last boulder into a platform to prevent any softlock
    Entity* last_boulder = world.map(MAP_ROUTE_GUMI_RYUMA_BOULDER)->entity(6);
    last_boulder->entity_type_id(world.entity_type("large_thin_yellow_platform")->id());
    last_boulder->entity_to_use_tiles_from(nullptr);
    last_boulder->position().half_z = true;
    last_boulder->palette(0);

    // Always remove Pockets from Gumi boulder map
    world.map(MAP_ROUTE_GUMI_RYUMA_BOULDER)->remove_entity(4);

    // Erase the first entity mask for each bear which makes them only appear under a specific scenario requirement
    std::vector<EntityMaskFlag>& first_bear_masks = world.map(MAP_ROUTE_GUMI_RYUMA_BOULDER)->entity(2)->mask_flags();
    first_bear_masks.erase(first_bear_masks.begin());
    
    std::vector<EntityMaskFlag>& second_bear_masks = world.map(MAP_ROUTE_GUMI_RYUMA_BOULDER)->entity(3)->mask_flags();
    second_bear_masks.erase(second_bear_masks.begin());
}

/**
 * The "falling ribbon" item source in Mercator castle court is pretty dependant on story flags. In the original game,
 * the timeframe in the story where we can get it is tight. We get rid of any condition here, apart from checking
 * if item has already been obtained.
 */
void make_falling_ribbon_not_story_dependant(World& world)
{
    // Remove use of map variant where the ribbon does not exist, and empty the now unused variant
    world.map(MAP_MERCATOR_CASTLE_EXTERIOR_LEFT_COURT)->variants().clear();
    world.map(MAP_MERCATOR_CASTLE_EXTERIOR_LEFT_COURT_RIBBONLESS_VARIANT)->clear();

    // Remove an entity mask preventing us from getting the ribbon once we get too far in the story
    std::vector<EntityMaskFlag>& ribbon_masks = world.map(MAP_MERCATOR_CASTLE_EXTERIOR_LEFT_COURT)->entity(2)->mask_flags();
    ribbon_masks.erase(ribbon_masks.begin() + ribbon_masks.size()-1);

    // Remove the servant guarding the door
    world.map(MAP_MERCATOR_CASTLE_EXTERIOR_LEFT_COURT)->remove_entity(0);
}