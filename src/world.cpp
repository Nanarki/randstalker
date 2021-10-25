#include "world.hpp"

#include "model/item.hpp"
#include "model/item_source.hpp"

#include "huffman/symbols.hpp"
#include "huffman/text_encoder.hpp"

#include "assets.hpp"
#include "game_text.hpp"
#include "exceptions.hpp"
#include "world_solver.hpp"

// Include headers automatically generated from model json files
#include "model/item.json.hxx"
#include "model/world_region.json.hxx"
#include "model/world_path.json.hxx"
#include "model/item_source.json.hxx"
#include "model/spawn_location.json.hxx"
#include "model/hint_source.json.hxx"
#include "model/world_macro_region.json.hxx"
#include "model/world_teleport_tree.json.hxx"

World::World(const RandomizerOptions& options) :
    _options                (options),
    _active_spawn_location  (nullptr),
    _dark_region            (nullptr)
{
    this->init_items();
    this->init_regions();
    this->init_item_sources();
    this->init_paths();
    this->init_macro_regions();
    this->init_spawn_locations();
    this->init_hint_sources();
    this->init_tree_maps();
}

World::~World()
{
	for (auto& [key, item] : _items)
		delete item;
    for (auto& [key, region] : _regions)
		delete region;
    for (auto& [key, path] : _paths)
		delete path;
    for (ItemSource* source : _item_sources)
		delete source;
    for (auto& [key, spawn_loc] : _spawn_locations)
		delete spawn_loc;
    for (auto& [key, hint_source] : _hint_sources)
		delete hint_source;
    for (WorldMacroRegion* macro : _macro_regions)
		delete macro;
    for (WorldTeleportTree* teleport_tree : _teleport_trees)
		delete teleport_tree;
}

Item* World::item(const std::string& name) const
{
    if(name.empty())
        return nullptr;

    for (auto& [key, item] : _items)
        if(item->name() == name)
            return item;

    return nullptr;
}

Item* World::add_gold_item(uint8_t worth)
{
    uint8_t highest_item_id = _items.rbegin()->first;

    // Try to find an item with the same worth
    for(uint8_t i=ITEM_GOLDS_START ; i<=highest_item_id ; ++i)
        if(_items[i]->gold_value() == worth)
            return _items[i];

    // If we consumed all item IDs, don't add it you fool!
    if(highest_item_id == 0xFF)
        return nullptr;

    return this->add_item(new ItemGolds(highest_item_id+1, worth));
}

WorldRegion* World::first_region_with_item(Item* item)
{
    for (auto& [key, region] : _regions)
    {
        std::vector<ItemSource*> sources = region->item_sources();
        for (ItemSource* source : sources)
            if (source->item() == item)
                return region;
    }

    return nullptr;
}


WorldPath* World::path(WorldRegion* origin, WorldRegion* destination)
{
    return _paths.at(std::make_pair(origin, destination));
}

WorldPath* World::path(const std::string& origin_name, const std::string& destination_name)
{
    WorldRegion* origin = _regions.at(origin_name);
    WorldRegion* destination = _regions.at(destination_name);
    return this->path(origin, destination);
}

void World::add_path(WorldPath* path)
{
    _paths[std::make_pair(path->origin(), path->destination())] = path;
}


std::vector<ItemSource*> World::item_sources_with_item(Item* item)
{
	std::vector<ItemSource*> sources_with_item;

	for (auto& [key, region] : _regions)
	{
		std::vector<ItemSource*> sources = region->item_sources();
		for (ItemSource* source : sources)
			if (source->item() == item)
				sources_with_item.push_back(source);
    }

    return sources_with_item;
}


void World::init_items()
{
    Json items_json = Json::parse(ITEMS_JSON);
    for(const Json& item_json : items_json)
        this->add_item(Item::from_json(item_json));
    std::cout << _items.size() << " items loaded." << std::endl;

    if (_options.useArmorUpgrades())
    {
        _items[ITEM_STEEL_BREAST]->gold_value(250);
        _items[ITEM_CHROME_BREAST]->gold_value(250);
        _items[ITEM_SHELL_BREAST]->gold_value(250);
        _items[ITEM_HYPER_BREAST]->gold_value(250);
    }

    if (_options.consumableRecordBook())
    {
        _items[ITEM_RECORD_BOOK]->max_quantity(9);
        uint16_t currentPrice = _items[ITEM_RECORD_BOOK]->gold_value();
        _items[ITEM_RECORD_BOOK]->gold_value(currentPrice / 5);
    }

    // Process custom starting quantities for items
    const std::map<std::string, uint8_t>& startingItems = _options.getStartingItems();
    for(auto& [item_name, quantity] : startingItems)
    {
        Item* item = this->item(item_name);
        if(!item)
        {
            std::stringstream msg;
            msg << "Cannot set starting quantity of unknown item '" << item_name << "'";
            throw RandomizerException(msg.str());
        }

        item->starting_quantity(std::min<uint8_t>(quantity, 9));
    }

    // Process custom item prices
    const std::map<std::string, uint16_t>& itemPrices = _options.getItemPrices();
    for(auto& [item_name, price] : itemPrices)
    {
        Item* item = this->item(item_name);
        if(!item)
        {
            std::stringstream msg;
            msg << "Cannot set starting price of unknown item '" << item_name << "'";
            throw RandomizerException(msg.str());
        }

        item->gold_value(price);
    }

    // Process custom item max quantities
    const std::map<std::string, uint8_t>& itemMaxQuantities = _options.getItemMaxQuantities();
    for(auto& [item_name, maxQuantity] : itemMaxQuantities)
    {
        Item* item = this->item(item_name);
        if(!item)
        {
            std::stringstream msg;
            msg << "Cannot set max quantity of unknown item '" << item_name << "'";
            throw RandomizerException(msg.str());
        }

        item->max_quantity(maxQuantity);
    }

    if(_options.getJewelCount() > MAX_INDIVIDUAL_JEWELS)
    {
        _items[ITEM_RED_JEWEL]->name("Kazalt Jewel");
        _items[ITEM_RED_JEWEL]->allowed_on_ground(false);
        _items[ITEM_RED_JEWEL]->max_quantity(_options.getJewelCount());
    }
}

void World::init_item_sources()
{
    Json item_sources_json = Json::parse(ITEM_SOURCES_JSON);
    for(const Json& source_json : item_sources_json)
    {
        _item_sources.push_back(ItemSource::from_json(source_json, _regions));
    }
    std::cout << _item_sources.size() << " item sources loaded." << std::endl;

    // The following chests are absent from the game on release or modded out of the game for the rando, and their IDs are therefore free:
    // 0x0E, 0x1E, 0x20, 0x25, 0x27, 0x28, 0x33, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0xA8, 0xBB, 0xBC, 0xBD, 0xBE, 0xC3
}

void World::init_regions()
{
    Json regions_json = Json::parse(WORLD_REGIONS_JSON);
    for(const Json& region_json : regions_json)
    {
        WorldRegion* new_region = WorldRegion::from_json(region_json);
        _regions[new_region->id()] = new_region;
    }
    std::cout << _regions.size() << " regions loaded." << std::endl;
}

void World::init_paths()
{
    Json paths_json = Json::parse(WORLD_PATHS_JSON);
    for(const Json& path_json : paths_json)
    {
        this->add_path(WorldPath::from_json(path_json, _regions, _items));

        if(path_json.contains("twoWay") && path_json.at("twoWay"))
        {
            Json inverted_json = path_json;
            inverted_json["fromId"] = path_json.at("toId");
            inverted_json["toId"] = path_json.at("fromId");
            this->add_path(WorldPath::from_json(inverted_json, _regions, _items));
        }
    }
    std::cout << _paths.size() << " paths loaded." << std::endl;   

    // Determine the list of required jewels to go from King Nole's Cave to Kazalt depending on settings
    WorldPath* path_to_kazalt = this->path("king_nole_cave", "kazalt");
    if(_options.getJewelCount() > MAX_INDIVIDUAL_JEWELS)
    {
        for(int i=0; i<_options.getJewelCount() ; ++i)
            path_to_kazalt->add_required_item(_items[ITEM_RED_JEWEL]);
    }
    else if(_options.getJewelCount() >= 1)
    {
        path_to_kazalt->add_required_item(_items[ITEM_RED_JEWEL]);
        if(_options.getJewelCount() >= 2)
            path_to_kazalt->add_required_item(_items[ITEM_PURPLE_JEWEL]);
        if(_options.getJewelCount() >= 3)
            path_to_kazalt->add_required_item(_items[ITEM_GREEN_JEWEL]);
        if(_options.getJewelCount() >= 4)
            path_to_kazalt->add_required_item(_items[ITEM_BLUE_JEWEL]);
        if(_options.getJewelCount() >= 5)
            path_to_kazalt->add_required_item(_items[ITEM_YELLOW_JEWEL]);
    }

    // Handle paths related to speicfic tricks
    if(_options.handleGhostJumpingInLogic())
    {
        WorldRegion* mountainous_area = _regions.at("mountainous_area");
        WorldRegion* route_lake_shrine = _regions.at("route_lake_shrine");
        this->add_path(new WorldPath(route_lake_shrine, mountainous_area, 1, { _items[ITEM_AXE_MAGIC] }));
    }
}

void World::init_macro_regions()
{
    Json macro_regions_json = Json::parse(WORLD_MACRO_REGIONS_JSON);
    for(const Json& macro_region_json : macro_regions_json)
        _macro_regions.push_back(WorldMacroRegion::from_json(macro_region_json, _regions));
    std::cout << _macro_regions.size() << " macro regions loaded." << std::endl;
}

void World::init_spawn_locations()
{
    Json spawns_json = Json::parse(SPAWN_LOCATIONS_JSON);
    for(const Json& spawn_json : spawns_json)
        this->add_spawn_location(SpawnLocation::from_json(spawn_json, _regions));
    std::cout << _spawn_locations.size() << " spawn locations loaded." << std::endl;
}

void World::init_hint_sources()
{
    Json hint_sources_json = Json::parse(HINT_SOURCES_JSON, nullptr, true, true);
    for(const Json& hint_source_json : hint_sources_json)
    {
        HintSource* new_source = HintSource::from_json(hint_source_json, _regions);
        _hint_sources[new_source->description()] = new_source;
    }
    std::cout << _hint_sources.size() << " hint sources loaded." << std::endl;
}

void World::init_tree_maps()
{
    Json trees_json = Json::parse(WORLD_TELEPORT_TREES_JSON);
    for(const Json& tree_json : trees_json)
        _teleport_trees.push_back(WorldTeleportTree::from_json(tree_json));

    std::cout << _hint_sources.size() << " teleport trees loaded." << std::endl;
}

void World::write_to_rom(md::ROM& rom)
{
    // Write a data block for gold values
    uint8_t highestGoldItemID = _items.rbegin()->first;
    uint8_t goldItemsCount = (highestGoldItemID - ITEM_GOLDS_START) + 1;
    uint32_t addr = rom.reserveDataBlock(goldItemsCount, "data_gold_values");
    for(uint8_t item_id = ITEM_GOLDS_START ; item_id <= highestGoldItemID ; ++item_id, ++addr)
        rom.setByte(addr, static_cast<uint8_t>(_items.at(item_id)->gold_value()));

    // Write item info
    for (auto& [key, item] : _items)
		item->write_to_rom(rom);

    // Write item sources' contents
	for (ItemSource* source : _item_sources)
		source->write_to_rom(rom);

    // Alter game text lines
    std::vector<std::string> textLines; 
    loadGameStrings(textLines);

    // Kazalt rejection message
    textLines[0x022] = std::string("Only the bearers of the ") 
        + std::to_string(_options.getJewelCount()) + " jewels\n are worthy of entering\n King Nole's domain...\x1E";

    // Hint sources
    for (auto& [k, hint_source]: _hint_sources)
    {
        std::vector<uint16_t> text_ids = hint_source->text_ids();
    	textLines[text_ids[0]] = GameText(hint_source->text()).getOutput();
        for(auto it=text_ids.begin()+1 ; it != text_ids.end() ; ++it)
            textLines[*it] = " ";
    }

    // Write all text lines into text banks
    TextEncoder encoder(rom, textLines);

    // Inject dark rooms as a data block
    const std::vector<uint16_t>& darkRooms = _dark_region->dark_map_ids();
    uint16_t darkRoomsByteCount = static_cast<uint16_t>(darkRooms.size() + 1) * 0x02;
    uint32_t darkRoomsArrayAddress = rom.reserveDataBlock(darkRoomsByteCount, "data_dark_rooms");
    uint8_t i = 0;
    for (uint16_t roomID : darkRooms)
        rom.setWord(darkRoomsArrayAddress + (i++) * 0x2, roomID);
    rom.setWord(darkRoomsArrayAddress + i * 0x2, 0xFFFF);

    // Write Tibor tree map connections
	for (WorldTeleportTree* teleport_tree : _teleport_trees)
		teleport_tree->write_to_rom(rom);
}



Json World::to_json() const
{
	Json json;

    // Export dark region
    json["spawnLocation"] = _active_spawn_location->id();
    json["darkRegion"] = _dark_region->name();

    // Export hints
    for(auto& [description, source] : _hint_sources)
		json["hints"][description] = source->text();

    // Export item sources
	for(auto& it : _regions)
	{
		const WorldRegion& region = *it.second;
		for(ItemSource* source : region.item_sources())
		{
            Item* item = _items.at(source->item_id());
			json["itemSources"][region.name()][source->name()] = item->name();
		}
	}

    return json;
}

void World::parse_json(const Json& json)
{
    ////////// Item Sources ///////////////////////////////////////////
    const Json& itemSourcesJson = json.at("itemSources");
	for(auto& it : _regions)
	{
		const WorldRegion& region = *it.second;
        if(region.item_sources().empty())
            continue;

        if(itemSourcesJson.contains(region.name()))
        {
            const Json& regionJson = itemSourcesJson.at(region.name());
            for(ItemSource* source : region.item_sources())
            {
                if(regionJson.contains(source->name()))
                {
                    std::string item_name = regionJson.at(source->name());
                    Item* item = this->parseItemFromName(item_name);
                    if(item)
                    {
                        source->item(item);
                    }
                    else
                    {
                        std::stringstream msg;
                        msg << "Item name '" << item_name << "' is invalid in plando JSON.";
                        throw RandomizerException(msg.str());
                    }
                }
                else
                {
                    std::stringstream msg;
                    msg << "Item source '" << source->name() << "' is missing from plando JSON.";
                    throw RandomizerException(msg.str());
                }
            }
        }
        else
        {
            std::stringstream msg;
            msg << "Region '" << region.name() << "' is missing from plando JSON.";
            throw RandomizerException(msg.str());
        }
	}

    ////////// Hints ///////////////////////////////////////////
    const Json& hintsJson = json.at("hints");
    for(auto& [description, source] : _hint_sources)
    {
        if(hintsJson.contains(description))
        {
            if(hintsJson.at(description).is_array())
            {
                std::vector<std::string> hint_lines;
                for(const std::string& line : hintsJson.at(description))
                    hint_lines.push_back(line);
                std::string hint = Tools::join(hint_lines, "\n");
                source->text(hint);
            }
            else
                source->text(hintsJson.at(description));
        }
        else
        {
            std::stringstream msg;
            msg << "Hint source '" << description << "' is missing from plando JSON.";
            throw RandomizerException(msg.str());
        }
    }

    ////////// Miscellaneous ///////////////////////////////////////////
    if(json.contains("spawnLocation"))
    {
        try {
            _active_spawn_location = _spawn_locations.at(json.at("spawnLocation"));
        } 
        catch(std::out_of_range&)
        {
            std::stringstream msg;
            msg << "Spawn location name '" << json.at("spawnLocation") << "' is invalid in plando JSON.";
            throw JsonParsingException(msg.str());
        }
    }

    if(json.contains("darkRegion"))
    {
        std::string darkRegionName = json.at("darkRegion");
        _dark_region = this->region(darkRegionName);
        if(!_dark_region)
        {
            std::stringstream msg;
            msg << "Darkened region name '" << darkRegionName << "' is invalid in plando JSON.";
            throw JsonParsingException(msg.str());
        }
    }
    else throw RandomizerException("Darkened region is missing from plando JSON.");
}

Item* World::parseItemFromName(const std::string& item_name)
{
    Item* item = this->item(item_name);
    if(item)
        return item;

    // If item is formatted as "X golds", parse X value and create the matching gold stack item
    if(Tools::endsWith(item_name, "golds"))
    {
        size_t spaceIndex = item_name.find_first_of(' ');
        if(spaceIndex == std::string::npos)
            return nullptr;

        std::string numberPart = item_name.substr(0, spaceIndex);
        
        try {
            uint8_t goldValue = static_cast<uint8_t>(std::strtol(numberPart.c_str(), nullptr, 10));
            return this->add_gold_item(goldValue);
        }
        catch(std::exception&) {}
    }

    return nullptr;
}

std::vector<Item*> World::findSmallestInventoryToReachRegion(WorldRegion* endRegion) const
{
    WorldRegion* spawnRegion = _active_spawn_location->region();
    WorldSolver solver(spawnRegion, endRegion);
    solver.tryToSolve();
    std::vector<Item*> inventory = solver.getInventory();

    std::vector<Item*> minimalInventory;

    UnsortedSet<Item*> forbiddenItems;

    for(Item* item : inventory)
    {  
        UnsortedSet<Item*> forbiddenItemsPlusOne = forbiddenItems;
        forbiddenItemsPlusOne.insert(item);
        
        WorldSolver solver2(spawnRegion, endRegion);
        solver2.forbidItems(forbiddenItemsPlusOne);
        if(solver2.tryToSolve())
        {
            // Item can be freely removed: keep it removed for further solves
            forbiddenItems = forbiddenItemsPlusOne;
        }
        else
        {
            // Item cannot be removed: it means it's required
            minimalInventory.push_back(item);
        }
    }
 
    return minimalInventory;
}

bool World::is_macro_region_avoidable(WorldMacroRegion* macroRegion) const
{
    WorldRegion* spawnRegion = _active_spawn_location->region();
    WorldRegion* endRegion = _regions.at("end");
    WorldSolver solver(spawnRegion, endRegion);
    solver.forbidTakingItemsFromRegions(macroRegion->regions());
    
    return solver.tryToSolve();
}

bool World::is_item_avoidable(Item* item) const
{
    WorldRegion* spawnRegion = _active_spawn_location->region();
    WorldRegion* endRegion = _regions.at("end");
    WorldSolver solver(spawnRegion, endRegion);
    solver.forbidItems({ item });

    return solver.tryToSolve();
}