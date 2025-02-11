#include "randomizer_world.hpp"

#include <landstalker_lib/model/world_teleport_tree.hpp>
#include <landstalker_lib/model/spawn_location.hpp>
#include <landstalker_lib/model/world.hpp>
#include <landstalker_lib/exceptions.hpp>
#include "world_node.hpp"
#include "world_path.hpp"
#include "world_region.hpp"
#include "hint_source.hpp"
#include "item_distribution.hpp"
#include "data/world_node.json.hxx"
#include "data/world_path.json.hxx"
#include "data/world_region.json.hxx"
#include "data/spawn_location.json.hxx"
#include "data/hint_source.json.hxx"
#include "data/item_distribution.json.hxx"

#include <iostream>

RandomizerWorld::RandomizerWorld(const md::ROM& rom) : World(rom)
{
    this->load_nodes();
    this->load_paths();
    this->load_regions();
    this->load_spawn_locations();
    this->load_hint_sources();
    this->load_item_distributions();
}

RandomizerWorld::~RandomizerWorld()
{
    for (auto& [key, node] : _nodes)
        delete node;
    for (auto& [key, path] : _paths)
        delete path;
    for (WorldRegion* region : _regions)
        delete region;
    for (HintSource* hint_source : _hint_sources)
        delete hint_source;
    for (auto& [key, spawn_loc] : _available_spawn_locations)
        delete spawn_loc;
}

void RandomizerWorld::load_nodes()
{
    Json nodes_json = Json::parse(WORLD_NODES_JSON);
    for(auto& [node_id, node_json] : nodes_json.items())
    {
        WorldNode* new_node = WorldNode::from_json(node_id, node_json);
        _nodes[node_id] = new_node;
    }

    for(ItemSource* source : this->item_sources())
    {
        const std::string& node_id = source->node_id();
        try {
            _nodes.at(node_id)->add_item_source(source);
        } catch(std::out_of_range&) {
            throw LandstalkerException("Could not find node '" + node_id + "' referenced by item source '" + source->name() + "'");
        }
    }

    std::cout << _nodes.size() << " nodes loaded." << std::endl;
}

void RandomizerWorld::load_paths()
{
    Json paths_json = Json::parse(WORLD_PATHS_JSON);
    for(const Json& path_json : paths_json)
    {
        this->add_path(WorldPath::from_json(path_json, _nodes, this->items()));

        if(path_json.contains("twoWay") && path_json.at("twoWay"))
        {
            Json inverted_json = path_json;
            inverted_json["fromId"] = path_json.at("toId");
            inverted_json["toId"] = path_json.at("fromId");
            this->add_path(WorldPath::from_json(inverted_json, _nodes, this->items()));
        }
    }
    std::cout << _paths.size() << " paths loaded." << std::endl;
}

void RandomizerWorld::load_regions()
{
    Json regions_json = Json::parse(WORLD_REGIONS_JSON);
    for(const Json& region_json : regions_json)
        _regions.emplace_back(WorldRegion::from_json(region_json, _nodes));

    std::cout << _regions.size() << " regions loaded." << std::endl;

    for(auto& [id, node] : _nodes)
        if(node->region() == nullptr)
            throw LandstalkerException("Node '" + node->id() + "' doesn't belong to any region");
}

void RandomizerWorld::load_spawn_locations()
{
    // Load base model
    Json spawns_json = Json::parse(SPAWN_LOCATIONS_JSON);
    for(auto& [id, spawn_json] : spawns_json.items())
        this->add_spawn_location(SpawnLocation::from_json(id, spawn_json));
    std::cout << _available_spawn_locations.size() << " spawn locations loaded." << std::endl;
}

void RandomizerWorld::load_hint_sources()
{
    Json hint_sources_json = Json::parse(HINT_SOURCES_JSON, nullptr, true, true);
    for(const Json& hint_source_json : hint_sources_json)
    {
        HintSource* new_source = HintSource::from_json(hint_source_json, _nodes);
        _hint_sources.emplace_back(new_source);
    }
    std::cout << _hint_sources.size() << " hint sources loaded." << std::endl;
}

void RandomizerWorld::load_item_distributions()
{
    Json item_distributions_json = Json::parse(ITEM_DISTRIBUTIONS_JSON, nullptr, true, true);
    for(auto& [item_id_str, item_distribution_json] : item_distributions_json.items())
    {
        uint8_t item_id = std::stoi(item_id_str);
        if(_item_distributions.count(item_id))
            throw LandstalkerException("Cannot load multiple item distributions for item id " + std::to_string((uint16_t)item_id));

        _item_distributions[item_id] = ItemDistribution::from_json(item_distribution_json);
    }
    std::cout << _item_distributions.size() << " item distributions loaded." << std::endl;
}

WorldPath* RandomizerWorld::path(WorldNode* origin, WorldNode* destination)
{
    return _paths.at(std::make_pair(origin, destination));
}

WorldPath* RandomizerWorld::path(const std::string& origin_name, const std::string& destination_name)
{
    WorldNode* origin = _nodes.at(origin_name);
    WorldNode* destination = _nodes.at(destination_name);
    return this->path(origin, destination);
}

void RandomizerWorld::add_path(WorldPath* path)
{
    _paths[std::make_pair(path->origin(), path->destination())] = path;
}

WorldRegion* RandomizerWorld::region(const std::string& name) const
{
    for(WorldRegion* region : _regions)
        if(region->name() == name)
            return region;
    return nullptr;
}

void RandomizerWorld::add_spawn_location(SpawnLocation* spawn)
{
    _available_spawn_locations[spawn->id()] = spawn;
}

void RandomizerWorld::dark_region(WorldRegion* region)
{
    _dark_region = region;
    this->dark_maps(region->dark_map_ids());

    for(WorldNode* node : region->nodes())
    {
        for (WorldPath* path : node->ingoing_paths())
            if(path->origin()->region() != region)
                path->add_required_item(this->item(ITEM_LANTERN));

        node->add_hint("in a very dark place");
    }
}

void RandomizerWorld::spawn_location(const SpawnLocation& spawn)
{
    World::spawn_location(spawn);
    _spawn_node = _nodes.at(spawn.node_id());
}

std::map<uint8_t, uint16_t> RandomizerWorld::item_quantities() const
{
    std::map<uint8_t, uint16_t> item_quantities;
    for(uint8_t i=0 ; i<=ITEM_GOLDS_START ; ++i)
        item_quantities[i] = 0;

    for(auto& [item_id, item_distrib] : _item_distributions)
        item_quantities[item_id] = item_distrib->quantity();

    return item_quantities;
}

HintSource* RandomizerWorld::hint_source(const std::string& name) const
{
    for(HintSource* source : _hint_sources)
        if(source->description() == name)
            return source;

    throw LandstalkerException("Could not find hint source '" + name + "' as requested");
}
