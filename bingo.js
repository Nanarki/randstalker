var minigameGoals = [
    { name: "Play Greenpea's boulder game 3 times", types: [""] },
    { name: "Play Greenpea's boulder game 5 times", types: [""] },
    { name: "Win the chicken race minigame at the casino", types: [""] },
    { name: "Throw 5+ chicken in the pit in a game of chicken toss at the casino", types: [""] },
    // + Roulette
];

var goofyItemGoals = [
    { name: "Win a beauty contest using the Blue Ribbon", types: [""] },
    { name: "Use Death Statue", types: [""] },
    { name: "Give Shortcake to Friday", types: [""] },
    { name: "Use Pawn Ticket", types: [""] },
    { name: "Get Casino Ticket", types: [""] },
    { name: "Get Lantern", types: [""] },
    { name: "Get Bell", types: [""] },
    { name: "Use Spell Book", types: [""] },
    { name: "Get Statue of Jypta", types: [""] },
];

var getOptionalItemGoals = [
    { name: "Get Einstein Whistle", types: [""] },
    { name: "Get Garlic", types: [""] },
    { name: "Get both Garlics", types: [""] },
    { name: "Get Logs", types: [""] },
    { name: "Get Buyer's Card", types: [""] },
    { name: "Get Idol Stone", types: [""] },
    { name: "Get Armlet", types: [""] },
];

var getKeyItemGoals = [
    { name: "Get Gola's Eye", types: [""] },
    { name: "Get Gola's Nail", types: [""] },
    { name: "Get Gola's Horn", types: [""] },
    { name: "Get Gola's Fang", types: [""] },
    { name: "Get Safety Pass", types: [""] },
    { name: "Get Red Jewel", types: [""] },
    { name: "Get Purple Jewel", types: [""] },
    { name: "Get Sun Stone", types: [""] },
    { name: "Get Key", types: [""] },
    { name: "Get Axe Magic", types: [""] },
];

var getBootsOrRingGoals = [
    { name: "Get Iron Boots", types: ["equipment", "boots"] },
    { name: "Get Snow Spikes", types: ["equipment", "boots"] },
    { name: "Get Fireproof Boots", types: ["equipment", "boots"] },
    { name: "Get Healing Boots", types: ["equipment", "boots"] },
    { name: "Get Venus Stone", types: ["equipment", "ring"] },
    { name: "Get Saturn Stone", types: ["equipment", "ring"] },
    { name: "Get Mars Stone", types: ["equipment", "ring"] },
    { name: "Get Moon Stone", types: ["equipment", "ring"] },
];

var getSwordOrArmorGoals = [
    { name: "Get Magic Sword", types: ["equipment", "sword"] },
    { name: "Get Thunder Sword", types: ["equipment", "sword"] },
    { name: "Get Ice Sword", types: ["equipment", "sword"] },
    { name: "Get Sword of Gaia", types: ["equipment", "sword"] },
    { name: "Get Steel Breast", types: ["equipment", "armor"] },
    { name: "Get Chrome Breast", types: ["equipment", "armor"] },
    { name: "Get Shell Breast", types: ["equipment", "armor"] },
    { name: "Get Hyper Breast", types: ["equipment", "armor"] },
];

var getAllEquipmentsOfTypeGoals = [
    { name: "Get all 4 swords", types: ["equipment", "sword"] },
    { name: "Get all 4 armors", types: ["equipment", "armor"] },
    { name: "Get all 4 rings", types: ["equipment", "ring"] },
    { name: "Get all 4 boots", types: ["equipment", "boots"] },
];

var sleepInInnsGoals = [
    { name: "Sleep in 2 different inns", types: ["inn"] },
    { name: "Sleep in 3 different inns", types: ["inn"] },
    { name: "Sleep in 4 different inns", types: ["inn"] },
    { name: "Sleep in 5 different inns", types: ["inn"] },
];

var haveGoldGoals = [
    { name: "Have 600 gold", types: [""] },
    { name: "Have 800 gold", types: [""] },
    { name: "Have 1000 gold", types: [""] },
    { name: "Have 1200 gold", types: [""] },
];

var reachTotalLifeGoals = [
    { name: "Reach 30 total life", types: [""] },
    { name: "Reach 40 total life", types: [""] },
    { name: "Reach 50 total life", types: [""] },
    { name: "Reach 60 total life", types: [""] },
    { name: "Reach 70 total life", types: [""] },
];

var getNineConsumablesGoals = [
    { name: "Get 9x EkeEke", types: [""] },
    { name: "Get 9x Detox Grass", types: [""] },
    { name: "Get 9x Dahl", types: [""] },
    { name: "Get 9x Anti-Paralyze", types: [""] },
    { name: "Get 9x Restoration", types: [""] },
    { name: "Get 9x Statue of Gaia", types: [""] },
    { name: "Get 9x Golden Statue", types: [""] },
];

var enterDungeonGoals = [
    { name: "Enter Waterfall Shrine", types: [""] },
    { name: "Enter Massan Cave", types: [""] },
    { name: "Enter Swamp Shrine", types: [""] },
    { name: "Enter Thieves Hideout", types: [""] },
    { name: "Enter Mercator Dungeon", types: [""] },
    { name: "Enter Verla Mines", types: [""] },
    { name: "Enter Destel Well", types: [""] },
    { name: "Enter Lake Shrine", types: [""] },
    { name: "Teleport to Kazalt", types: [""] },
];

var buyOneItemInShopsGoals = [
    { name: "Buy an item in 4 different shops", types: ["shop"] },
    { name: "Buy an item in 5 different shops", types: ["shop"] },
    { name: "Buy an item in 6 different shops", types: ["shop"] },
    { name: "Buy an item in 7 different shops", types: ["shop"] },
    { name: "Buy an item in 8 different shops", types: ["shop"] },
];

var buyEveryItemInShopGoals = [
    { name: "Buy each item at least once in Massan shop", types: ["shop"] },
    { name: "Buy each item at least once in Gumi shop", types: ["shop"] },
    { name: "Buy each item at least once in Ryuma main shop", types: ["shop"] },
    { name: "Buy each item at least once in Mercator town shop", types: ["shop"] },
    { name: "Buy each item at least once in Mercator docks shop", types: ["shop"] },
    { name: "Buy each item at least once in Verla shop", types: ["shop"] },
    { name: "Buy each item at least once in the waterfall shop", types: ["shop"] },
    { name: "Buy each item at least once in Destel main shop", types: ["shop"] },
    { name: "Buy each item at least once in Greedly's shop", types: ["shop"] },
    { name: "Buy each item at least once in Kazalt shop", types: ["shop"] },
    { name: "Buy each item at least once in Mercator special shop", types: ["shop"] },
];

var easyBossesGoals = [
    { name: "Beat Orc Kings in Swamp Shrine", types: ["boss"] },
    { name: "Beat Silver Knight in Thieves Hideout", types: ["boss"] },
    { name: "Beat Golem in Destel Well", types: ["boss"] },
    { name: "Beat the Duke in Lake Shrine", types: ["boss"] },
];

var hardBossesGoals = [
    { name: "Beat King Nole", types: ["boss"] },
    { name: "Beat Firedemon", types: ["boss"] },
    { name: "Beat Miro twice", types: ["boss"] },
    { name: "Beat Mir", types: ["boss"] },
    { name: "Beat Zac", types: ["boss"] },
];

var sidequestGoals = [
    { name: "Free Tibor", types: [""] },
    { name: "Call the lumberjack using the whistle", types: [""] },
    { name: "Fix the lighthouse using the Sun Stone", types: [""] },
];

var painfulCheckGoals = [
    { name: "Morph back to human form after being a dog", types: [""] },
    { name: "Beat Fahl in his dojo", types: [""] },
    { name: "Beat the Mummy in the Crypt", types: [""] },
    { name: "Beat Slasher in Verla", types: [""] },
];

var enterBigTreeGoals = [
    { name: "Enter big tree near Ryuma", types: [""] },
    { name: "Enter big tree near Massan", types: [""] },
    { name: "Enter big tree near Mercator gate", types: [""] },
    { name: "Enter big tree near Verla", types: [""] },
    { name: "Enter big tree near Mir Tower", types: [""] },
    { name: "Enter big tree in Mountainous Area", types: [""] },
    { name: "Enter big tree near Destel", types: [""] },
    { name: "Enter big tree near Lake Shrine", types: [""] },
];

var beatThreeVariantsofEnemyGoals = [
    { name: "Beat 3 color variants of Orcs", types: [""] },
    { name: "Beat 3 color variants of Mushrooms", types: [""] },
    { name: "Beat 3 color variants of Slimes", types: [""] },
    { name: "Beat 3 color variants of Cyclops", types: [""] },
    { name: "Beat 3 color variants of Knights", types: [""] },
    { name: "Beat 3 color variants of Mummies", types: [""] },
    { name: "Beat 3 color variants of Lizards", types: [""] },
];

var fancyTrickGoals = [
    { name: "Jump on Massan statue (no dog throw)", types: [""] },
    { name: "Break all jars inside grandma's house in Mercator", types: [""] },
    { name: "Use a Dahl while at 1 HP", types: [""] },
    { name: "Cure a debuff using a book in a church", types: [""] },
];

var MISSING_1 = [

];

var MISSING_2 = [

];

var MISSING_3 = [

];

var MISSING_4 = [
    
];

var bingoList = [
    /* 0 */ getKeyItemGoals, 
    /* 1 */ getBootsOrRingGoals,
    /* 2 */ getSwordOrArmorGoals,
    /* 3 */ getNineConsumablesGoals,
    /* 4 */ getOptionalItemGoals,
    /* 5 */ enterDungeonGoals,
    /* 6 */ sleepInInnsGoals,
    /* 7 */ fancyTrickGoals,
    /* 8 */ haveGoldGoals,
    /* 9 */ goofyItemGoals,
    /* 10 */ buyOneItemInShopsGoals,
    /* 11 */ enterBigTreeGoals,
    /* 12 */ minigameGoals,
    /* 13 */ MISSING_1,
    /* 14 */ sidequestGoals,
    /* 15 */ easyBossesGoals,
    /* 16 */ MISSING_2,
    /* 17 */ beatThreeVariantsofEnemyGoals,
    /* 18 */ reachTotalLifeGoals,
    /* 19 */ MISSING_3,
    /* 20 */ MISSING_4,
    /* 21 */ painfulCheckGoals,
    /* 22 */ buyEveryItemInShopGoals,
    /* 23 */ getAllEquipmentsOfTypeGoals,
    /* 24 */ hardBossesGoals,
];

// Beat a boss without anything equipped