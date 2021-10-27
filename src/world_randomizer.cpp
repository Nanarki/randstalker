#include "world_randomizer.hpp"

#include <algorithm>
#include <sstream>

#include "tools/tools.hpp"
#include "tools/game_text.hpp"

#include "exceptions.hpp"
#include "world_solver.hpp"

WorldRandomizer::WorldRandomizer(World& world, const RandomizerOptions& options) :
    _world            (world),
    _options          (options),
    _rng              (),
    _gold_items_count (0),
    _debug_log_json   (options.to_json())
{
    this->init_filler_items();
    this->init_mandatory_items();
    this->init_inventory();
}

void WorldRandomizer::randomize()
{
    uint32_t rngSeed = _options.seed();

    // 1st pass: randomizations happening BEFORE randomizing items
    _rng.seed(rngSeed);
    this->randomize_spawn_location();
    this->randomize_gold_values();
    this->randomize_dark_rooms();

    // 2nd pass: randomizing items
    _rng.seed(rngSeed);
    this->randomize_items();

    // Analyse items required to complete the seed
    _minimal_items_to_complete = _world.minimal_inventory_to_complete();
    _debug_log_json["requiredItems"] = Json::array();
    for (Item* item : _minimal_items_to_complete)
        _debug_log_json["requiredItems"].push_back(item->name());

    // 3rd pass: randomizations happening AFTER randomizing items
    _rng.seed(rngSeed);
    this->randomize_hints();

    if(_options.shuffle_tibor_trees())
        this->randomize_tibor_trees();
}

void WorldRandomizer::init_filler_items()
{
    std::map<std::string, uint16_t> filler_items_desc;
    if(_options.has_custom_filler_items())
    {
        filler_items_desc = _options.filler_items();
    }
    else
    {
        filler_items_desc = { 
            {"Life Stock", 80},      {"EkeEke", 55},         {"Golds", 30},          {"Dahl", 16},             
            {"Statue of Gaia", 12},  {"Detox Grass", 11},    {"Golden Statue", 10},  {"Restoration", 10},     
            {"Mind Repair", 7},      {"Anti Paralyze", 7},   {"No Item", 4},         {"Pawn Ticket", 1},
            {"Short Cake", 1},       {"Bell", 1},            {"Blue Ribbon", 1},     {"Death Statue", 1}
        };
    }

    for (auto& [item_name, quantity] : filler_items_desc)
    {
        if(item_name == "Golds")
        {
            _gold_items_count += quantity;
            continue;
        }

        Item* item = _world.item(item_name);
        if(!item)
        {
            std::stringstream msg;
            msg << "Unknown item '" << item_name << "' found in filler items.";
            throw RandomizerException(msg.str());
        }
        
        for(uint16_t i=0 ; i<quantity ; ++i)
            _filler_items.push_back(item);
    }
}

void WorldRandomizer::init_mandatory_items()
{
    std::map<std::string, uint16_t> mandatory_items_desc;
    if(_options.has_custom_mandatory_items())
    {
        mandatory_items_desc = _options.mandatory_items();
    }
    else
    {
        mandatory_items_desc = {
            {"Magic Sword", 1},      {"Thunder Sword", 1},     {"Sword of Ice", 1},     {"Sword of Gaia", 1},
            {"Steel Breast", 1},     {"Chrome Breast", 1},     {"Shell Breast", 1},     {"Hyper Breast", 1},
            {"Healing Boots", 1},    {"Iron Boots", 1},        {"Fireproof", 1},
            {"Mars Stone", 1},       {"Moon Stone", 1},        {"Saturn Stone", 1},     {"Venus Stone", 1},
            {"Oracle Stone", 1},     {"Statue of Jypta", 1},   {"Spell Book", 1},
        };
    }

    for (auto& [item_name, quantity] : mandatory_items_desc)
    {
        Item* item = _world.item(item_name);
        if(!item)
        {
            std::stringstream msg;
            msg << "Unknown item '" << item_name << "' found in mandatory items.";
            throw RandomizerException(msg.str());
        }
        
        for(uint16_t i=0 ; i<quantity ; ++i)
            _mandatoryItems.push_back(item);
    }
}

void WorldRandomizer::init_inventory()
{
    for(const auto& [id, item] : _world.items())
    {
        uint8_t quantity = item->starting_quantity();
        for(uint8_t i=0 ; i<quantity ; ++i)
            _inventory.push_back(item);
    }
}


///////////////////////////////////////////////////////////////////////////////////////
///        FIRST PASS RANDOMIZATIONS (before items)
///////////////////////////////////////////////////////////////////////////////////////

void WorldRandomizer::randomize_spawn_location()
{
    std::vector<std::string> possible_spawn_locations = _options.possible_spawn_locations();
    if(possible_spawn_locations.empty())
    {
        for(auto& [id, spawn] : _world.spawn_locations())
            possible_spawn_locations.push_back(id);
    }

    Tools::shuffle(possible_spawn_locations, _rng);
    SpawnLocation* spawn = _world.spawn_locations().at(possible_spawn_locations[0]);
    _world.active_spawn_location(spawn);
}

void WorldRandomizer::randomize_gold_values()
{
    constexpr uint16_t AVERAGE_GOLD_PER_CHEST = 35;
    constexpr double MAX_FACTOR_OF_TOTAL_GOLD = 0.16;

    uint16_t total_gold = AVERAGE_GOLD_PER_CHEST * _gold_items_count;

    for (uint8_t i = 0; i < _gold_items_count ; ++i)
    {
        uint16_t gold_value;

        if (i < _gold_items_count - 1)
        {
            double proportion = (double) _rng() / (double) _rng.max();
            double factor = (proportion * MAX_FACTOR_OF_TOTAL_GOLD);

            gold_value = (uint16_t)((double)total_gold * factor);
        }
        else
        {
            gold_value = total_gold;
        }

        if (gold_value == 0)
            gold_value = 1;
        else if (gold_value > 255)
            gold_value = 255;

        total_gold -= gold_value;

        Item* gold_item = _world.add_gold_item(static_cast<uint8_t>(gold_value));
        if(gold_item)
            _filler_items.push_back(gold_item);
    }
}

void WorldRandomizer::randomize_dark_rooms()
{
    std::vector<WorldRegion*> possible_regions;
    for (auto& [key, region] : _world.regions())
        if (!region->dark_map_ids().empty())
            possible_regions.push_back(region);

    Tools::shuffle(possible_regions, _rng);
    _world.dark_region(possible_regions[0]);

    const std::vector<WorldPath*>& ingoing_paths = _world.dark_region()->ingoing_paths();
    for (WorldPath* path : ingoing_paths)
        path->add_required_item(_world.item(ITEM_LANTERN));
}



///////////////////////////////////////////////////////////////////////////////////////
///        SECOND PASS RANDOMIZATIONS (items)
///////////////////////////////////////////////////////////////////////////////////////

void WorldRandomizer::randomize_items()
{
    _regions_to_explore = { _world.active_spawn_location()->region() };
    _explored_regions.clear();        // Regions already processed by the exploration algorithm
    _item_sources_to_fill.clear();        // Reachable empty item sources which must be filled with a random item
    _inventory.clear();        // The current contents of player inventory at the given time in the exploration algorithm
    _pending_paths.clear();            // Paths leading to potentially unexplored regions, locked behind a key item which is not yet owned by the player

    Tools::shuffle(_filler_items, _rng);

    this->place_mandatory_items();

    uint32_t step_count = 1;
    while (!_pending_paths.empty() || !_regions_to_explore.empty())
    {
        Json& debug_step_json = _debug_log_json["steps"][std::to_string(step_count)];

        // Explore not yet explored regions, listing all item sources and paths for further exploration and processing
        this->exploration_phase(debug_step_json);

        // Try unlocking paths and item sources with newly discovered items in pre-filled item sources (useful for half-plando)
        this->unlock_phase();

        Tools::shuffle(_item_sources_to_fill, _rng);

        // Place one or several key items to unlock access to a path, opening new regions & item sources
        this->place_key_items_phase(debug_step_json);

        // Fill a fraction of already available sources with filler items
        if (!_item_sources_to_fill.empty())
        {
            size_t sourcesToFillCount = (size_t)(_item_sources_to_fill.size() * _options.filling_rate());
            this->place_filler_items_phase(debug_step_json, sourcesToFillCount);
        }

        // Try unlocking paths and item sources with the newly acquired key item
        this->unlock_phase();
        ++step_count;
    }

    // Place the remaining filler items, and put Ekeeke in the last empty sources
    Json& debug_step_json = _debug_log_json["steps"]["remainder"];
    this->place_filler_items_phase(debug_step_json, _item_sources_to_fill.size(), _world.item(ITEM_EKEEKE));

    if (!_filler_items.empty())
    {
        debug_step_json["unplacedItems"] = Json::array();
        for (Item* item : _filler_items)
            debug_step_json["unplacedItems"].push_back(item->name());
    }

    _debug_log_json["endState"]["remainingSourcesToFill"] = Json::array();
    for(auto source : _item_sources_to_fill)
        _debug_log_json["endState"]["remainingSourcesToFill"].push_back(source->name());
    _debug_log_json["endState"]["pending_paths"] = _pending_paths.size();
}

void WorldRandomizer::place_mandatory_items()
{
    _debug_log_json["steps"]["0"]["comment"] = "Placing mandatory items";

    // Mandatory items are filler items which are always placed first in the randomization, no matter what
    Tools::shuffle(_mandatoryItems, _rng);

    std::vector<ItemSource*> all_empty_item_sources;
    for (ItemSource* source : _world.item_sources())
    {
        if(!source->item())
            all_empty_item_sources.push_back(source);
    }
    Tools::shuffle(all_empty_item_sources, _rng);

    for (Item* item_to_place : _mandatoryItems)
    {
        for (ItemSource* source : all_empty_item_sources)
        {
            if (!source->item() && source->is_item_compatible(item_to_place))
            {
                source->item(item_to_place);
                _debug_log_json["steps"]["0"]["placedItems"][source->name()] = item_to_place->name();
                break;
            }
        }
    }
}

void WorldRandomizer::place_filler_items_phase(Json& debug_step_json, size_t count, Item* last_resort_filler)
{
    for (size_t i=0 ; i<count ; ++i)
    {
        ItemSource* source = _item_sources_to_fill[0];
        
        for (size_t j=0 ; j < _filler_items.size(); ++j)
        {
            Item* fillerItem = _filler_items[j];
            if (source->is_item_compatible(fillerItem))
            {
                source->item(fillerItem);
                _filler_items.erase(_filler_items.begin() + j);
                _item_sources_to_fill.erase(_item_sources_to_fill.begin());
                break;
            }
        }

        if(source->item() == nullptr)
        {
            // No valid item could be put inside the itemSource...
            if(last_resort_filler)
            {
                // Fill with the "last resort filler" if provided
                source->item(last_resort_filler);
                _item_sources_to_fill.erase(_item_sources_to_fill.begin());
            }
            else
            {
                // No last resort provided, put this item source on the back of the list
                _item_sources_to_fill.erase(_item_sources_to_fill.begin());
                _item_sources_to_fill.push_back(source);
            }
        }

        if(source->item())
            debug_step_json["filledSources"][source->name()] = source->item()->name();
    }
}

void WorldRandomizer::exploration_phase(Json& debug_step_json)
{
    debug_step_json["exploredRegions"] = Json::array();

    while (!_regions_to_explore.empty())
    {
        // Take and erase first region from regions to explore, add it to explored regions set.
        WorldRegion* explored_region = *_regions_to_explore.begin();
        _regions_to_explore.erase(explored_region);
        _explored_regions.insert(explored_region);
        debug_step_json["exploredRegions"].push_back(explored_region->name());

        // List item sources to fill from this region.
        const std::vector<ItemSource*> item_sources = explored_region->item_sources();
        for (ItemSource* source : item_sources)
        {
            if(source->item())
                _inventory.push_back(source->item());    // Non-empty item sources populate player inventory (useful for plandos)
            else
                _item_sources_to_fill.push_back(source);
        }

        // List outgoing paths
        for (WorldPath* outgoing_path : explored_region->outgoing_paths())
        {
            // If destination is already pending exploration (through another path) or has already been explored, just ignore it
            WorldRegion* destination = outgoing_path->destination();
            if (_regions_to_explore.contains(destination) || _explored_regions.contains(destination))
                continue;

            bool can_cross = true;
            // Path cannot be taken if we don't have all off the required items
            if(!outgoing_path->missing_items_to_cross(_inventory).empty())
                can_cross = false;
            // Path cannot be taken if we don't have already explored other required regions
            if(!outgoing_path->has_explored_required_regions(_explored_regions))
                can_cross = false;

            if(can_cross)
            {
                // For crossable paths, add destination to the list of regions to explore
                _regions_to_explore.insert(destination);
            }
            else
            {
                // For uncrossable blocked paths, add them to a pending list
                _pending_paths.push_back(outgoing_path);
            }
        }
    }
}

void WorldRandomizer::place_key_items_phase(Json& debug_step_json)
{
    if (_pending_paths.empty())
        return;

    // List all unowned key items, and pick a random one among them
    std::vector<WorldPath*> blocked_paths;
    for (WorldPath* pending_path : _pending_paths)
    {
        // Don't consider this path for key item placement if unexplored regions are a blocking criteria
        if(!pending_path->has_explored_required_regions(_explored_regions))
            continue;

        if(!pending_path->missing_items_to_cross(_inventory).empty())
        {
            // If items are missing to cross this path, add as many entries as the weight of the path to the blocked_paths array
            for(int i=0 ; i<pending_path->weight() ; ++i)
                blocked_paths.push_back(pending_path);
        }
    }

    Tools::shuffle(blocked_paths, _rng);
    WorldPath* pathToOpen = blocked_paths[0];
    std::vector<Item*> missing_key_items = pathToOpen->missing_items_to_cross(_inventory);
    for(Item* key_item_to_place : missing_key_items)
    {
        // Find a random item source capable of carrying the item
        ItemSource* random_item_source = nullptr;
        for (uint32_t i = 0; i < _item_sources_to_fill.size(); ++i)
        {
            if (_item_sources_to_fill[i]->is_item_compatible(key_item_to_place))
            {
                random_item_source = _item_sources_to_fill[i];
                _item_sources_to_fill.erase(_item_sources_to_fill.begin() + i);
                break;
            }
        }
        if (!random_item_source)
            throw NoAppropriateItemSourceException();

        // Place the key item in the appropriate source, and also add it to player inventory
        random_item_source->item(key_item_to_place);
        _logical_playthrough.push_back(random_item_source);
        _inventory.push_back(key_item_to_place);

        debug_step_json["placedKeyItems"][random_item_source->name()] = key_item_to_place->name();
    }
}

void WorldRandomizer::unlock_phase()
{
    // Look for unlockable paths...
    for (size_t i=0 ; i < _pending_paths.size() ; ++i)
    {
        WorldPath* pending_path = _pending_paths[i];

        // Path cannot be taken if we don't have already explored other required regions
        if(!pending_path->has_explored_required_regions(_explored_regions))
            continue;
        // Path cannot be taken if we don't have all of the required items
        if(!pending_path->missing_items_to_cross(_inventory).empty())
            continue;

        // Unlock path, add destination to regions to explore if it has not yet been explored
        WorldRegion* destination = pending_path->destination();
        if (!_regions_to_explore.contains(destination) && !_explored_regions.contains(destination))
            _regions_to_explore.insert(destination);

        // Remove path from pending paths
        _pending_paths.erase(_pending_paths.begin()+i);
        --i;
    }
}


///////////////////////////////////////////////////////////////////////////////////////
///        THIRD PASS RANDOMIZATIONS (after items)
///////////////////////////////////////////////////////////////////////////////////////

void WorldRandomizer::randomize_hints()
{
    this->randomize_lithograph_hint();
    this->randomize_where_is_lithograph_hint();

    Item* hinted_fortune_item = this->randomize_fortune_teller_hint();
    Item* hinted_oracle_stone_item = this->randomize_oracle_stone_hint(hinted_fortune_item);

    this->randomize_sign_hints(hinted_fortune_item, hinted_oracle_stone_item);
}

void WorldRandomizer::randomize_lithograph_hint()
{
    std::stringstream lithograph_hint;
    if(_options.jewel_count() > MAX_INDIVIDUAL_JEWELS)
    {
        bool first = true;
        std::vector<ItemSource*> sources_containing_jewels = _world.item_sources_with_item(_world.item(ITEM_RED_JEWEL));
        for(ItemSource* source : sources_containing_jewels)
        {
            if(first)
                first = false;
            else
                lithograph_hint << "\n";
            lithograph_hint << "A jewel is " << this->random_hint_for_item_source(source) << ".";
        }
    }
    else if(_options.jewel_count() >= 1)
    {
        lithograph_hint << "Red Jewel is " << this->random_hint_for_item(_world.item(ITEM_RED_JEWEL)) << ".";
        if(_options.jewel_count() >= 2)
            lithograph_hint << "\nPurple Jewel is " << this->random_hint_for_item(_world.item(ITEM_PURPLE_JEWEL)) << ".";
        if(_options.jewel_count() >= 3)
            lithograph_hint << "\nGreen Jewel is " << this->random_hint_for_item(_world.item(ITEM_GREEN_JEWEL)) << ".";
        if(_options.jewel_count() >= 4)
            lithograph_hint << "\nBlue Jewel is " << this->random_hint_for_item(_world.item(ITEM_BLUE_JEWEL)) << ".";
        if(_options.jewel_count() >= 5)
            lithograph_hint << "\nYellow Jewel is " << this->random_hint_for_item(_world.item(ITEM_YELLOW_JEWEL)) << ".";
    }
    else
        lithograph_hint << "This tablet seems of no use...";

    _world.hint_sources().at("Lithograph")->text(lithograph_hint.str());
}

void WorldRandomizer::randomize_where_is_lithograph_hint()
{
    std::stringstream where_is_litho_hint;
    where_is_litho_hint << "The lithograph will help you finding the jewels. It is "
                        << this->random_hint_for_item(_world.item(ITEM_LITHOGRAPH))
                        << ".";

    _world.hint_sources().at("King Nole's Cave sign")->text(where_is_litho_hint.str());
}

Item* WorldRandomizer::randomize_fortune_teller_hint()
{
    std::vector<uint8_t> hintable_items = { ITEM_GOLA_EYE, ITEM_GOLA_NAIL, ITEM_GOLA_FANG, ITEM_GOLA_HORN };
    Tools::shuffle(hintable_items, _rng);
    
    Item* hinted_item = _world.item(*(hintable_items.begin()));

    std::string item_fancy_name;
    if (hinted_item == _world.item(ITEM_GOLA_EYE))
        item_fancy_name = "an eye";
    else if (hinted_item == _world.item(ITEM_GOLA_NAIL))
        item_fancy_name = "a nail";
    else if (hinted_item == _world.item(ITEM_GOLA_FANG))
        item_fancy_name = "a fang";
    else if (hinted_item == _world.item(ITEM_GOLA_HORN))
        item_fancy_name = "a horn";

    std::stringstream fortune_teller_hint;
    fortune_teller_hint << "\x1cI see... \x1aI see... \x1a\nI see " << item_fancy_name << " " << this->random_hint_for_item(hinted_item) << ".";
    _world.hint_sources().at("Mercator fortune teller")->text(fortune_teller_hint.str());

    return hinted_item;
}

Item* WorldRandomizer::randomize_oracle_stone_hint(Item* forbidden_fortune_teller_item)
{
    UnsortedSet<Item*> forbidden_items = {
        forbidden_fortune_teller_item, _world.item(ITEM_RED_JEWEL), _world.item(ITEM_PURPLE_JEWEL),
        _world.item(ITEM_GREEN_JEWEL), _world.item(ITEM_BLUE_JEWEL), _world.item(ITEM_YELLOW_JEWEL)
    };

    // Also excluding items strictly needed to get to Oracle Stone's location
    std::vector<ItemSource*> sources = _world.item_sources_with_item(_world.item(ITEM_ORACLE_STONE));
    WorldRegion* first_source_region = sources.at(0)->region();
    if(first_source_region)
    {
        std::vector<Item*> min_items_to_reach = _world.minimal_inventory_to_reach(first_source_region);
        for (Item* item : min_items_to_reach)
            forbidden_items.insert(item);
    }

    std::vector<Item*> hintable_items;
    for (Item* item : _minimal_items_to_complete)
    {
        if(!forbidden_items.contains(item))
            hintable_items.push_back(item);
    }
    
    if (!hintable_items.empty())
    {
        Tools::shuffle(hintable_items, _rng);
        Item* hinted_item = hintable_items[0];

        std::stringstream oracle_stone_hint;
        oracle_stone_hint << "You will need " << hinted_item->name() << ". It is " << this->random_hint_for_item(hinted_item) << ".";
        _world.hint_sources().at("Oracle Stone")->text(oracle_stone_hint.str());

        return hinted_item;
    }
    
    _world.hint_sources().at("Oracle Stone")->text("The stone looks blurry. It looks like it won't be of any use...");
    return nullptr;
}


void WorldRandomizer::randomize_sign_hints(Item* hinted_fortune_item, Item* hinted_oracle_stone_item)
{
    // A shuffled list of macro regions, used for the "barren / useful region" hints
    UnsortedSet<WorldMacroRegion*> hintable_macro_regions = _world.macro_regions();
    Tools::shuffle(hintable_macro_regions, _rng);

    // A shuffled list of potentially optional items, useful for the "this item will be useful / useless" hints
    UnsortedSet<uint8_t> hintable_item_requirements = {
        ITEM_BUYER_CARD,   ITEM_EINSTEIN_WHISTLE,   ITEM_ARMLET,    ITEM_GARLIC, 
        ITEM_IDOL_STONE,   ITEM_CASINO_TICKET,      ITEM_LOGS
    };
    Tools::shuffle(hintable_item_requirements, _rng);

    // A shuffled list of items which location is interesting, useful for the "item X is in Y" hints
    UnsortedSet<uint8_t> hintable_item_locations = {
        ITEM_SPIKE_BOOTS,       ITEM_AXE_MAGIC,      ITEM_BUYER_CARD,    ITEM_GARLIC,
        ITEM_EINSTEIN_WHISTLE,  ITEM_ARMLET,         ITEM_IDOL_STONE,
        ITEM_THUNDER_SWORD,     ITEM_HEALING_BOOTS,  ITEM_VENUS_STONE,   ITEM_STATUE_JYPTA,
        ITEM_SUN_STONE,         ITEM_KEY,            ITEM_SAFETY_PASS,   ITEM_LOGS,
        ITEM_GOLA_EYE,          ITEM_GOLA_NAIL,      ITEM_GOLA_FANG,     ITEM_GOLA_HORN
    };
    Tools::shuffle(hintable_item_locations, _rng);

    // Remove items that have been already hinted by special hints
    if(hinted_fortune_item)
        hintable_item_locations.erase(hinted_fortune_item->id());
    if(hinted_oracle_stone_item)
        hintable_item_locations.erase(hinted_oracle_stone_item->id());

    for (auto& [k, hint_source] : _world.hint_sources())
    {
        // Hint source is special (e.g. Oracle Stone, Lithograph...), don't handle it here
        if(hint_source->special())
            continue;

        WorldRegion* region = hint_source->region();
        UnsortedSet<Item*> inventory_at_sign = _world.minimal_inventory_to_reach(region);
        
        double randomNumber = (double) _rng() / (double) _rng.max();

        ////////////////////////////////////////////////////////////////
        // "Barren / pleasant surprise" (30%)
        if (randomNumber < 0.3 && !hintable_macro_regions.empty())
        {
            WorldMacroRegion* macroRegion = hintable_macro_regions[0];
            hintable_macro_regions.erase(macroRegion);

            if (_world.is_macro_region_avoidable(macroRegion))
                hint_source->text("What you are looking for is not in " + macroRegion->name() + ".");
            else
                hint_source->text("You might have a pleasant surprise wandering in " + macroRegion->name() + ".");

            continue;
        }

        ////////////////////////////////////////////////////////////////
        // "You will / won't need {item} to finish" (25%)
        if (randomNumber < 0.55)
        {
            Item* hinted_item_requirement = nullptr;
            for(uint8_t item_id : hintable_item_requirements)
            {
                Item* tested_item = _world.item(item_id);
                if(!inventory_at_sign.contains(tested_item))
                {
                    // If item was not already obtained at sign, we can hint it
                    hinted_item_requirement = tested_item;
                    hintable_item_requirements.erase(item_id);
                    break;
                }
            }

            if(hinted_item_requirement)
            {
                if (!_world.is_item_avoidable(hinted_item_requirement))
                    hint_source->text("You will need " + hinted_item_requirement->name() + " in your quest to King Nole's treasure.");
                else
                    hint_source->text(hinted_item_requirement->name() + " is not required in your quest to King Nole's treasure.");
                continue;
            }
        }

        ////////////////////////////////////////////////////////////////
        // "You shall find {item} in {place}" (45%)
        if (!hintable_item_locations.empty())
        {
            Item* hinted_item_location = nullptr;
            for(uint8_t item_id : hintable_item_locations)
            {
                Item* tested_item = _world.item(item_id);
                if(!inventory_at_sign.contains(tested_item))
                {
                    // If item was not already obtained at sign, we can hint it
                    hinted_item_location = tested_item;
                    hintable_item_locations.erase(item_id);
                    break;
                }
            }

            if (hinted_item_location)
            {
                hint_source->text("You shall find " + hinted_item_location->name() + " " + this->random_hint_for_item(hinted_item_location) + ".");
                continue;
            }
        }

        // Fallback if none matched
        hint_source->text("This sign has been damaged in a way that makes it unreadable.");
    }
}

std::string WorldRandomizer::random_hint_for_item(Item* item)
{
    std::vector<ItemSource*> sources = _world.item_sources_with_item(item);
    if(sources.empty())
        return "in an unknown place";

    Tools::shuffle(sources, _rng);
    ItemSource* randomSource = sources[0];
    return this->random_hint_for_item_source(randomSource);
}

std::string WorldRandomizer::random_hint_for_item_source(ItemSource* itemSource)
{
    const std::vector<std::string>& region_hints = itemSource->region()->hints();
    const std::vector<std::string>& source_hints = itemSource->hints();
    
    std::vector<std::string> all_hints;
    all_hints.insert(all_hints.end(), region_hints.begin(), region_hints.end());
    all_hints.insert(all_hints.end(), source_hints.begin(), source_hints.end());
    
    if(all_hints.empty())
        return "in an unknown place";

    Tools::shuffle(all_hints, _rng);
    return all_hints[0];
}


void WorldRandomizer::randomize_tibor_trees()
{
    const std::vector<WorldTeleportTree*>& trees = _world.teleport_trees();

    std::vector<uint16_t> teleport_tree_map_ids;
    for (WorldTeleportTree* tree : trees)
        teleport_tree_map_ids.push_back(tree->tree_map_id());
    
    Tools::shuffle(teleport_tree_map_ids, _rng);
    for (uint8_t i = 0; i < trees.size(); ++i)
        trees.at(i)->tree_map_id(teleport_tree_map_ids[i]);
}

Json WorldRandomizer::playthrough_as_json() const
{
    Json json;

    // Filter the logical playthrough to keep only strictly needed key items
    for(ItemSource* source : _logical_playthrough)
    {
        Item* key_item_in_source = source->item();
        if(std::find(_minimal_items_to_complete.begin(), _minimal_items_to_complete.end(), key_item_in_source) != _minimal_items_to_complete.end())
            json[source->name()] = key_item_in_source->name();
    }

    return json;
}
