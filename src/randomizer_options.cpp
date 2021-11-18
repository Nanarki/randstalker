#include "randomizer_options.hpp"

#include <iostream>
#include "extlibs/base64.hpp"
#include "tools/tools.hpp"
#include "exceptions.hpp"
#include "tools/bitpack.hpp"

RandomizerOptions::RandomizerOptions() :
    _jewel_count                    (2),
    _use_armor_upgrades             (true),
    _startingLife                   (0),
    _startingGold                   (0),
    _starting_items                 ({{"Record Book",1}}),
    _fix_armlet_skip                (true),
    _fix_tree_cutting_glitch        (true),
    _consumable_record_book         (false),
    _remove_gumi_boulder            (false),
    _remove_tibor_requirement       (false),
    _all_trees_visited_at_start     (false),
    _enemies_damage_factor          (100),
    _enemies_health_factor          (100),
    _enemies_armor_factor           (100),
    _enemies_golds_factor           (100),
    _enemies_drop_chance_factor     (100),

    _seed                           (0),
    _allow_spoiler_log              (true),
    _filling_rate                   (20),
    _shuffle_tibor_trees            (false), 
    _ghost_jumping_in_logic         (false),
    _damage_boosting_in_logic       (false),
    _mandatory_items                (nullptr),
    _filler_items                   (nullptr),

    _add_ingame_item_tracker        (false),
    _hud_color                      ("default"),

    _plando_enabled                 (false),
    _plando_json                    ()
{}

RandomizerOptions::RandomizerOptions(const ArgumentDictionary& args) : RandomizerOptions()
{
    std::string plando_path = args.get_string("plando");
    if(!plando_path.empty())
    {
        _plando_enabled = true;

        std::cout << "Reading plando file '" << plando_path << "'...\n\n";
        std::ifstream plando_file(plando_path);
        if(!plando_file)
            throw RandomizerException("Could not open plando file at given path '" + plando_path + "'");

        plando_file >> _plando_json;
        if(_plando_json.contains("plando_permalink"))
            _plando_json = Json::from_msgpack(base64_decode(_plando_json.at("plando_permalink")));

        this->parse_json(_plando_json);
    }

    std::string permalink_string = args.get_string("permalink");
    if(!permalink_string.empty() && !_plando_enabled) 
    {
        this->parse_permalink(permalink_string);
    }
    else
    {
        // Parse seed from args, or generate a random one if it's missing
        std::string seed_string = args.get_string("seed", "random");
        try {
            _seed = (uint32_t) std::stoul(seed_string);
        } catch (std::invalid_argument&) {
            _seed = (uint32_t) std::chrono::system_clock::now().time_since_epoch().count();
        }

        std::string preset_path = args.get_string("preset");
        if(!preset_path.empty() && !_plando_enabled)
        {
            std::ifstream preset_file(preset_path);
            if(!preset_file)
                throw RandomizerException("Could not open preset file at given path '" + preset_path + "'");

            std::cout << "Reading preset file '" << preset_path << "'...\n\n";

            Json preset_json;
            preset_file >> preset_json;
            this->parse_json(preset_json);
        }

        this->parse_arguments(args);
    }

    this->parse_personal_settings(args);
    this->validate();
}

RandomizerOptions::~RandomizerOptions()
{
    if(_mandatory_items)
        delete _mandatory_items;
    if(_filler_items)
        delete _filler_items;
}

void RandomizerOptions::parse_arguments(const ArgumentDictionary& args)
{
    if(args.contains("spawnlocation"))
    {
        std::string spawn_name = args.get_string("spawnlocation");
        tools::to_lower(spawn_name);
        if(spawn_name == "random")
            _possible_spawn_locations = {};
        else
            _possible_spawn_locations = { spawn_name };
    }

    if(args.contains("jewelcount"))           _jewel_count = args.get_integer("jewelcount");
    if(args.contains("armorupgrades"))        _use_armor_upgrades = args.get_boolean("armorupgrades");
    if(args.contains("norecordbook"))         _starting_items["Record Book"] = 0;
    if(args.contains("startinglife"))         _startingLife = args.get_integer("startinglife");

    if(args.contains("fillingrate"))          _filling_rate = (uint8_t)(args.get_double("fillingrate") * 100);
    if(args.contains("shuffletrees"))         _shuffle_tibor_trees = args.get_boolean("shuffletrees");
    if(args.contains("allowspoilerlog"))      _allow_spoiler_log = args.get_boolean("allowspoilerlog");
}

void RandomizerOptions::parse_personal_settings(const ArgumentDictionary& args)
{
    // Personal options (not included in permalink)
    if(args.contains("ingametracker"))   _add_ingame_item_tracker = args.get_boolean("ingametracker");    
    if(args.contains("hudcolor"))        _hud_color = args.get_string("hudcolor");
}

Json RandomizerOptions::to_json() const
{
    Json json;

    // Game settings 
    json["gameSettings"]["jewelCount"] = _jewel_count;
    json["gameSettings"]["armorUpgrades"] = _use_armor_upgrades;
    json["gameSettings"]["startingGold"] = _startingGold;
    json["gameSettings"]["startingItems"] = _starting_items;
    json["gameSettings"]["fixArmletSkip"] = _fix_armlet_skip;
    json["gameSettings"]["fixTreeCuttingGlitch"] = _fix_tree_cutting_glitch;
    json["gameSettings"]["consumableRecordBook"] = _consumable_record_book;
    json["gameSettings"]["removeGumiBoulder"] = _remove_gumi_boulder;
    json["gameSettings"]["removeTiborRequirement"] = _remove_tibor_requirement;
    json["gameSettings"]["allTreesVisitedAtStart"] = _all_trees_visited_at_start;
    json["gameSettings"]["enemiesDamageFactor"] = _enemies_damage_factor;
    json["gameSettings"]["enemiesHealthFactor"] = _enemies_health_factor;
    json["gameSettings"]["enemiesArmorFactor"] = _enemies_armor_factor;
    json["gameSettings"]["enemiesGoldsFactor"] = _enemies_golds_factor;
    json["gameSettings"]["enemiesDropChanceFactor"] = _enemies_drop_chance_factor;
    if(_startingLife > 0)
        json["gameSettings"]["startingLife"] = _startingLife;

    // Randomizer settings
    json["randomizerSettings"]["allowSpoilerLog"] = _allow_spoiler_log;
    json["randomizerSettings"]["fillingRate"] = _filling_rate;
    json["randomizerSettings"]["spawnLocations"] = _possible_spawn_locations;
    json["randomizerSettings"]["shuffleTrees"] = _shuffle_tibor_trees;
    json["randomizerSettings"]["ghostJumpingInLogic"] = _ghost_jumping_in_logic;
    json["randomizerSettings"]["damageBoostingInLogic"] = _damage_boosting_in_logic;

    if(_mandatory_items)
        json["randomizerSettings"]["mandatoryItems"] = *_mandatory_items;
    if(_filler_items)
        json["randomizerSettings"]["fillerItems"] = *_filler_items;

    if(!_model_patch_items.empty())
        json["modelPatch"]["items"] = _model_patch_items;
    if(!_model_patch_spawns.empty())
        json["modelPatch"]["spawnLocations"] = _model_patch_spawns;

    return json;
}

void RandomizerOptions::parse_json(const Json& json)
{
    if(json.contains("gameSettings"))
    {
        const Json& game_settings_json = json.at("gameSettings");

        if(game_settings_json.contains("jewelCount"))            
            _jewel_count = game_settings_json.at("jewelCount");
        if(game_settings_json.contains("armorUpgrades"))
            _use_armor_upgrades = game_settings_json.at("armorUpgrades");
        if(game_settings_json.contains("startingLife"))
            _startingLife = game_settings_json.at("startingLife");
        if(game_settings_json.contains("startingGold"))
            _startingGold = game_settings_json.at("startingGold");
        if(game_settings_json.contains("fixArmletSkip"))
            _fix_armlet_skip = game_settings_json.at("fixArmletSkip");
        if(game_settings_json.contains("fixTreeCuttingGlitch"))
            _fix_tree_cutting_glitch = game_settings_json.at("fixTreeCuttingGlitch");
        if(game_settings_json.contains("consumableRecordBook"))
            _consumable_record_book = game_settings_json.at("consumableRecordBook");
        if(game_settings_json.contains("removeGumiBoulder"))
            _remove_gumi_boulder = game_settings_json.at("removeGumiBoulder");
        if(game_settings_json.contains("removeTiborRequirement"))
            _remove_tibor_requirement = game_settings_json.at("removeTiborRequirement");
        if(game_settings_json.contains("allTreesVisitedAtStart"))
            _all_trees_visited_at_start = game_settings_json.at("allTreesVisitedAtStart");
        if(game_settings_json.contains("enemiesDamageFactor"))
            _enemies_damage_factor = game_settings_json.at("enemiesDamageFactor");
        if(game_settings_json.contains("enemiesHealthFactor"))
            _enemies_health_factor = game_settings_json.at("enemiesHealthFactor");
        if(game_settings_json.contains("enemiesArmorFactor"))
            _enemies_armor_factor = game_settings_json.at("enemiesArmorFactor");
        if(game_settings_json.contains("enemiesGoldsFactor"))
            _enemies_golds_factor = game_settings_json.at("enemiesGoldsFactor");
        if(game_settings_json.contains("enemiesDropChanceFactor"))    
            _enemies_drop_chance_factor = game_settings_json.at("enemiesDropChanceFactor");

        if(game_settings_json.contains("startingItems"))
        {
            std::map<std::string, uint8_t> startingItems = game_settings_json.at("startingItems");
            for(auto& [itemName, quantity] : startingItems)
                _starting_items[itemName] = quantity;
        }
    }

    if(json.contains("randomizerSettings") && !_plando_enabled)
    {
        const Json& randomizer_settings_json = json.at("randomizerSettings");

        if(randomizer_settings_json.contains("allowSpoilerLog"))
            _allow_spoiler_log = randomizer_settings_json.at("allowSpoilerLog");
        if(randomizer_settings_json.contains("fillingRate"))
            _filling_rate = randomizer_settings_json.at("fillingRate");

        if(randomizer_settings_json.contains("spawnLocations"))
            randomizer_settings_json.at("spawnLocations").get_to(_possible_spawn_locations);
        else if(randomizer_settings_json.contains("spawnLocation"))
            _possible_spawn_locations = { randomizer_settings_json.at("spawnLocation") };

        if(randomizer_settings_json.contains("shuffleTrees"))
            _shuffle_tibor_trees = randomizer_settings_json.at("shuffleTrees");
        if(randomizer_settings_json.contains("ghostJumpingInLogic"))
            _ghost_jumping_in_logic = randomizer_settings_json.at("ghostJumpingInLogic");
        if(randomizer_settings_json.contains("damageBoostingInLogic"))
            _damage_boosting_in_logic = randomizer_settings_json.at("damageBoostingInLogic");

        if(randomizer_settings_json.contains("mandatoryItems"))
        {
            _mandatory_items = new std::map<std::string, uint16_t>();
            *(_mandatory_items) = randomizer_settings_json.at("mandatoryItems");
        }

        if(randomizer_settings_json.contains("fillerItems"))
        {
            _filler_items = new std::map<std::string, uint16_t>();
            *(_filler_items) = randomizer_settings_json.at("fillerItems");
        }
    }

    if(json.contains("modelPatch"))
    {
        const Json& model_patch_json = json.at("modelPatch");

        if(model_patch_json.contains("items"))
            _model_patch_items = model_patch_json.at("items");
        if(model_patch_json.contains("spawnLocations"))
            _model_patch_spawns = model_patch_json.at("spawnLocations");
    }

    if(json.contains("seed") && !_plando_enabled)
        _seed = json.at("seed");
}

Json RandomizerOptions::personal_settings_as_json() const
{
    Json json;

    json["addIngameItemTracker"] = _add_ingame_item_tracker;
    json["hudColor"] = _hud_color;

    return json;
}

void RandomizerOptions::validate()
{
    if(_jewel_count > 9)
        throw RandomizerException("Jewel count must be between 0 and 9.");
}

std::vector<std::string> RandomizerOptions::hash_words() const
{
    std::vector<std::string> words = { 
        "EkeEke",   "Nail",        "Horn",       "Fang",      "Magic",      "Ice",       "Thunder",    "Gaia",
        "Mars",     "Moon",        "Saturn",     "Venus",     "Detox",      "Statue",    "Golden",     "Mind",
        "Card",     "Lantern",     "Garlic",     "Paralyze",  "Chicken",    "Death",     "Jypta",      "Sun",
        "Book",     "Lithograph",  "Red",        "Purple",    "Jewel",      "Pawn",      "Gola",       "Nole",
        "Logs",     "Oracle",      "Stone",      "Idol",      "Key",        "Safety",    "Pass",       "Bell",
        "Massan",   "Gumi",        "Ryuma",      "Mercator",  "Verla",      "Destel",    "Kazalt",     "Greedly",
        "Mir",      "Miro",        "Prospero",   "Fara",      "Orc",        "Mushroom",  "Slime",      "Cyclops",
        "Kado",     "Kan",         "Well",       "Dungeon",   "Loria",      "Kayla",     "Wally",      "Ink",
        "Palace",   "Gold",        "Waterfall",  "Shrine",    "Swamp",      "Hideout",   "Greenmaze",  "Mines",
        "Helga",    "Fahl",        "Yard",       "Twinkle",   "Firedemon",  "Spinner",   "Golem",      "Boulder",
        "Kindly",   "Route",       "Shop",       "Green",     "Yellow",     "Blue",      "Fireproof",  "Iron",
        "Spikes",   "Healing",     "Snow",       "Repair",    "Casino",     "Ticket",    "Axe",        "Ribbon",
        "Armlet",   "Einstein",    "Whistle",    "Spell",     "King",       "Dragon",    "Dahl",       "Restoration", 
        "Friday",   "Short",       "Cake",       "Life",      "Stock",      "Zak",       "Duke",       "Dex", 
        "Slasher",  "Marley",      "Nigel",      "Ninja",     "Ghost",      "Tibor",     "Knight",     "Pockets", 
        "Arthur",   "Crypt",       "Mummy",      "Poison",    "Labyrinth",  "Lake",      "Volcano",    "Crate", 
        "Jar",      "Mayor",       "Dexter",     "Treasure",  "Chest"
    };

    std::mt19937 rng(_seed);
    tools::shuffle(words, rng);
    return std::vector<std::string>(words.begin(), words.begin() + 4);
}

std::string RandomizerOptions::permalink() const
{
    Bitpack bitpack;

    bitpack.pack(std::string(MAJOR_RELEASE));
    
    bitpack.pack(_jewel_count);
    bitpack.pack(_startingLife);
    bitpack.pack(_startingGold);
    bitpack.pack(_enemies_damage_factor);
    bitpack.pack(_enemies_health_factor);
    bitpack.pack(_enemies_armor_factor);
    bitpack.pack(_enemies_golds_factor);
    bitpack.pack(_enemies_drop_chance_factor);

    bitpack.pack(_seed);
    bitpack.pack(_filling_rate);

    bitpack.pack(_use_armor_upgrades);
    bitpack.pack(_fix_armlet_skip);
    bitpack.pack(_fix_tree_cutting_glitch);
    bitpack.pack(_consumable_record_book);
    bitpack.pack(_remove_gumi_boulder);
    bitpack.pack(_remove_tibor_requirement);
    bitpack.pack(_all_trees_visited_at_start);
    bitpack.pack(_allow_spoiler_log);
    bitpack.pack(_shuffle_tibor_trees);
    bitpack.pack(_ghost_jumping_in_logic);
    bitpack.pack(_damage_boosting_in_logic);

    bitpack.pack_vector(_possible_spawn_locations);
    bitpack.pack(_mandatory_items ? Json(*_mandatory_items) : Json());
    bitpack.pack(_filler_items ? Json(*_filler_items) : Json());
    bitpack.pack_map(_starting_items);
    bitpack.pack(_model_patch_items);
    bitpack.pack(_model_patch_spawns);
    
    return "l" + base64_encode(bitpack.to_bytes()) + "s";
}

void RandomizerOptions::parse_permalink(const std::string& permalink)
{
    std::vector<uint8_t> bytes = base64_decode(permalink.substr(1, permalink.size() - 2));
    Bitpack bitpack(bytes);

    std::string version = bitpack.unpack<std::string>();
    if(version != MAJOR_RELEASE)
        throw WrongVersionException("This permalink comes from an incompatible version of Randstalker (" + version + ").");
    
    _jewel_count = bitpack.unpack<uint8_t>();
    _startingLife = bitpack.unpack<uint8_t>();
    _startingGold = bitpack.unpack<uint16_t>();
    _enemies_damage_factor = bitpack.unpack<uint16_t>();
    _enemies_health_factor = bitpack.unpack<uint16_t>();
    _enemies_armor_factor = bitpack.unpack<uint16_t>();
    _enemies_golds_factor = bitpack.unpack<uint16_t>();
    _enemies_drop_chance_factor = bitpack.unpack<uint16_t>();

    _seed = bitpack.unpack<uint32_t>();
    _filling_rate = bitpack.unpack<uint8_t>();

    _use_armor_upgrades = bitpack.unpack<bool>();
    _fix_armlet_skip = bitpack.unpack<bool>();
    _fix_tree_cutting_glitch = bitpack.unpack<bool>();
    _consumable_record_book = bitpack.unpack<bool>();
    _remove_gumi_boulder = bitpack.unpack<bool>();
    _remove_tibor_requirement = bitpack.unpack<bool>();
    _all_trees_visited_at_start = bitpack.unpack<bool>();
    _allow_spoiler_log = bitpack.unpack<bool>();
    _shuffle_tibor_trees = bitpack.unpack<bool>();
    _ghost_jumping_in_logic = bitpack.unpack<bool>();
    _damage_boosting_in_logic = bitpack.unpack<bool>();

    _possible_spawn_locations = bitpack.unpack_vector<std::string>();

    Json mandatory_items_json = bitpack.unpack<Json>();
    if(!mandatory_items_json.is_null())
    {
        _mandatory_items = new std::map<std::string, uint16_t>();
        mandatory_items_json.get_to(*_mandatory_items);
    }
    else _mandatory_items = nullptr;

    Json filler_items_json = bitpack.unpack<Json>();
    if(!filler_items_json.is_null())
    {
        _filler_items = new std::map<std::string, uint16_t>();
        filler_items_json.get_to(*_filler_items); 
    }
    else _filler_items = nullptr;

    _starting_items = bitpack.unpack_map<std::string, uint8_t>();
    _model_patch_items = bitpack.unpack<Json>();
    _model_patch_spawns = bitpack.unpack<Json>();
}
