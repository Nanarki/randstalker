#include "GameAlterations.hpp"
#include "Enums/ItemCodes.hpp"
#include "RandomizerOptions.hpp"
#include "GameText.hpp"
#include "MegadriveTools/MdRom.hpp"
#include "MegadriveTools/MdCode.hpp"
#include "World.hpp"
#include "Exceptions.hpp"
#include "Assets.hpp"

#include <cstdint>
#include <vector>
#include <iostream>

constexpr uint32_t customTextStorageMemoryAddress = 0xFF0014;
constexpr uint32_t mapEntrancePositionStorageMemoryAddress = 0xFF0018;

///////////////////////////////////////////////////////////////////////////////////
//       Game & gameplay changes
///////////////////////////////////////////////////////////////////////////////////

void alterGameStart(md::ROM& rom, const RandomizerOptions& options, const World& world)
{
    // ------- Inject flags init function ---------
    // Init function used to set story flags to specific values at the very beginning of the game, opening some usually closed paths
    // and removing some useless cutscenes (considering them as "already seen").

    uint8_t flagArray[0x60];
    for(int i=0 ; i<0x60 ; ++i)
        flagArray[i] = 0x0;

    // Setup story flags
    flagArray[0x00] = 0xB1;
    flagArray[0x02] = 0xD5;
    flagArray[0x03] = 0xE0;
    flagArray[0x04] = 0xD5;
    flagArray[0x05] = 0x60;
    flagArray[0x06] = 0x7C;
    flagArray[0x07] = 0xB4;
    flagArray[0x08] = 0xFE;
    flagArray[0x09] = 0x7E;
    flagArray[0x12] = 0x1E;
    flagArray[0x13] = 0x43;
    flagArray[0x14] = 0x81;
    flagArray[0x17] = 0x48;
    flagArray[0x20] = 0x70;
    flagArray[0x26] = 0x80;
    flagArray[0x27] = 0x22;
    flagArray[0x28] = 0xE0;
    flagArray[0x29] = 0x40;
    flagArray[0x2A] = 0x81;
    flagArray[0x2B] = 0x82;

    // Clear Verla soldiers if spawning in Verla
    if(options.getSpawnLocation() == SpawnLocation::VERLA)
        flagArray[0x26] += 0x18;

    // Set starting items
    for(uint8_t itemID=0 ; itemID < ITEM_GOLDS_START ; itemID += 0x2)
    {
        uint8_t inventoryFlagValue = 0x0;

        uint8_t lshQuantity = world.items.at(itemID)->getStartingQuantity();
        if(lshQuantity)
            inventoryFlagValue |= (lshQuantity+1) & 0x0F;

        uint8_t mshQuantity = world.items.at(itemID+1)->getStartingQuantity();
        if(mshQuantity)
            inventoryFlagValue |= ((mshQuantity+1) & 0x0F) << 4;

        flagArray[0x40+(itemID/2)] = inventoryFlagValue;
    }

    // Setup inventory tracker if needed
    if (options.addIngameItemTracker())
    {
        flagArray[0x4B] |= 0x10;
        flagArray[0x4C] |= 0x10;
        flagArray[0x4D] |= 0x11;
        flagArray[0x4F] |= 0x11;
        flagArray[0x50] |= 0x01;
        flagArray[0x55] |= 0x10;
        flagArray[0x57] |= 0x10;
        flagArray[0x58] |= 0x10;
        flagArray[0x59] |= 0x11;
        flagArray[0x5B] |= 0x10;
        flagArray[0x5C] |= 0x11;
    }

    md::Code funcInitGame;

    // Set the orientation byte of Nigel depending on spawn location on game start
    funcInitGame.moveb(getSpawnLocationOrientation(world.spawnLocation), addr_(0xFF5404));

    for(int i=0 ; i<0x60 ; i+=0x2)
    {
        uint16_t value = (static_cast<uint16_t>(flagArray[i]) << 8) + static_cast<uint16_t>(flagArray[i+1]);
        if(value)
            funcInitGame.movew(value, addr_(0xFF1000+i));
    }

    funcInitGame.movew(options.getStartingGold(), addr_(0xFF120E));
    funcInitGame.rts();

    uint32_t funcInitGameAddr = rom.injectCode(funcInitGame);

    // ------- Set spawn position ---------
    rom.setWord(0x0027F4, getSpawnLocationMapID(world.spawnLocation));
    rom.setByte(0x0027FD, getSpawnLocationX(world.spawnLocation));
    rom.setByte(0x002805, getSpawnLocationZ(world.spawnLocation));

    // ------- Set spawn health ---------
    rom.setByte(0x0027B4, options.getStartingLife()-1);
    rom.setByte(0x0027BC, options.getStartingLife()-1);

    // ------- Remove no music flag ---------
    // Replace the bitset of the no music flag by a jump to the injected flags init function located at the end of the rom

    // 0x002700:
        // Before: 	[08F9] bset 3 -> $FF1027
        // After:	[4EB9] jsr $1FFAD0 ; [4E71] nop
    rom.setCode(0x002700, md::Code().jsr(funcInitGameAddr).nop());

    // ------- Remove cutscene flag (no input allowed) ---------
    // Usually, when starting a new game, it is automatically put into "cutscene mode" to let the intro roll without allowing the player
    // to move or pause, or do anything at all. We need to remove that cutscene flag to enable the player actually playing the game.

    // 0x00281A:
        // Before:	[33FC] move.w 0x00FE -> $FF12DE
        // After:	[4E71] nop (4 times)
    rom.setCode(0x281A, md::Code().nop(4));
}

void alterItemOrderInMenu(md::ROM& rom)
{
    std::vector<uint8_t> itemOrder = {
        ITEM_EKEEKE,        ITEM_RECORD_BOOK,
        ITEM_DAHL,          ITEM_RESTORATION,
        ITEM_GOLDEN_STATUE, ITEM_GAIA_STATUE,
        ITEM_DETOX_GRASS,   ITEM_MIND_REPAIR,
        ITEM_ANTI_PARALYZE, ITEM_ORACLE_STONE,
        ITEM_KEY,           ITEM_GARLIC,
        ITEM_LOGS,          ITEM_IDOL_STONE,
        ITEM_GOLA_EYE,      ITEM_GOLA_NAIL,
        ITEM_GOLA_HORN,     ITEM_GOLA_FANG,
        ITEM_DEATH_STATUE,  ITEM_STATUE_JYPTA,
        ITEM_SHORT_CAKE,    ITEM_PAWN_TICKET,
        ITEM_CASINO_TICKET, ITEM_LITHOGRAPH,
        ITEM_LANTERN,       ITEM_BELL,
        ITEM_SAFETY_PASS,   ITEM_ARMLET,
        ITEM_SUN_STONE,     ITEM_BUYER_CARD,
        ITEM_AXE_MAGIC,     ITEM_EINSTEIN_WHISTLE,
        ITEM_RED_JEWEL,     ITEM_PURPLE_JEWEL,
        ITEM_GREEN_JEWEL,   ITEM_BLUE_JEWEL,
        ITEM_YELLOW_JEWEL,  ITEM_SPELL_BOOK,
        ITEM_BLUE_RIBBON,   0xFF
    };

    uint32_t baseAddress = 0x00D55C;
    for (int i = 0; baseAddress + i < 0x00D584; ++i)
        rom.setByte(baseAddress + i, itemOrder[i]);
}

void alterLifestockHandlingInShops(md::ROM& rom)
{
    // Make Lifestock prices the same over all shops
    for (uint32_t addr = 0x024D34; addr <= 0x024EAE; addr += 0xE)
        rom.setByte(addr + 0x03, 0x10);

    // Remove the usage of "bought lifestock in shop X" flags 
    for (uint32_t addr = 0x009D18; addr <= 0x009D33; addr += 0xE)
        rom.setByte(addr, 0xFF);
}

void addStatueOfJyptaGoldsOverTime(md::ROM& rom)
{
    constexpr uint16_t goldsPerCycle = 0x0001;

    // ============== Function to handle walk abilities (healing boots, jypta statue...) ==============
    md::Code funcHandleWalkAbilities;

    // If Statue of Jypta is owned, gain gold over time
    funcHandleWalkAbilities.btst(0x5, addr_(0xFF104E));
    funcHandleWalkAbilities.beq(3);
    funcHandleWalkAbilities.movew(goldsPerCycle, reg_D0);
    funcHandleWalkAbilities.jsr(0x177DC);   // rom.getStoredAddress("func_earn_gold");

    // If Healing boots are equipped, gain life over time
    funcHandleWalkAbilities.cmpib(0x7, addr_(0xFF1150));
    funcHandleWalkAbilities.bne(4);
    funcHandleWalkAbilities.movew(0x100, reg_D0);
    funcHandleWalkAbilities.lea(0xFF5400, reg_A5);
    funcHandleWalkAbilities.jsr(0x1780E);   // rom.getStoredAddress("func_heal_hp");

    funcHandleWalkAbilities.rts();

    uint32_t funcAddr = rom.injectCode(funcHandleWalkAbilities);

    // ============== Hook the function inside game code ==============
    rom.setCode(0x16696, md::Code().nop(5));
    rom.setCode(0x166D0, md::Code().jsr(funcAddr).nop(4));
}

void quickenGaiaEffect(md::ROM& rom)
{
    constexpr uint8_t factor = 3;

    rom.setWord(0x1686C, rom.getWord(0x1686C) * factor);
    rom.setWord(0x16878, rom.getWord(0x16878) * factor);
    rom.setWord(0x16884, rom.getWord(0x16884) * factor);
}

void addRecordBookSave(md::ROM& rom, const RandomizerOptions& options)
{
    md::Code funcStoreCurrentMapAndPosition;
    funcStoreCurrentMapAndPosition.movew(addr_(0xFF1204), addr_(mapEntrancePositionStorageMemoryAddress));
    funcStoreCurrentMapAndPosition.movew(addr_(0xFF5400), addr_(mapEntrancePositionStorageMemoryAddress+2));
    funcStoreCurrentMapAndPosition.rts();
    uint32_t funcStoreCurrentMapAndPositionAddr = rom.injectCode(funcStoreCurrentMapAndPosition);

    // -------- Function to save game using record book --------

    // On record book use, set stored position and map, then call the save game function. Then, restore Nigel's position and map as if nothing happened.
    md::Code funcSaveUsingRecordBook;
    funcSaveUsingRecordBook.movemToStack({ reg_D0, reg_D1 }, {});

    if(options.consumableRecordBook())
    {
        funcSaveUsingRecordBook.moveb(reg_D0, addr_(0xFF1152));
        funcSaveUsingRecordBook.jsr(0x8B98); // ConsumeItem
    }

    funcSaveUsingRecordBook.movew(addr_(0xFF1204), reg_D0);
    funcSaveUsingRecordBook.movew(addr_(0xFF5400), reg_D1);
    
    funcSaveUsingRecordBook.movew(addr_(mapEntrancePositionStorageMemoryAddress), addr_(0xFF1204));
    funcSaveUsingRecordBook.movew(addr_(mapEntrancePositionStorageMemoryAddress+2), addr_(0xFF5400));
    funcSaveUsingRecordBook.jsr(0x1592); // "func_save_game"

    funcSaveUsingRecordBook.movew(reg_D0, addr_(0xFF1204));
    funcSaveUsingRecordBook.movew(reg_D1, addr_(0xFF5400));
    funcSaveUsingRecordBook.movemFromStack({ reg_D0, reg_D1 }, {});

    funcSaveUsingRecordBook.rts();
    rom.injectCode(funcSaveUsingRecordBook, "func_save_game_record_book");

    // -------- Procedure injections to store map entrance position --------
    
    // Regular map transition position storage injection
    md::Code procRegularMapTransition;
    procRegularMapTransition.movew(addr_(reg_A0, 0x4), addr_(0xFF5400));
    procRegularMapTransition.jsr(funcStoreCurrentMapAndPositionAddr);
    procRegularMapTransition.nop();
    rom.setCode(0xA0F6, procRegularMapTransition);

    // Update stored map and position on load
    md::Code funcLoad;
    funcLoad.jsr(0x15C2); // "func_load_game"
    funcLoad.jsr(funcStoreCurrentMapAndPositionAddr);
    funcLoad.rts();
    uint32_t funcLoadAddr = rom.injectCode(funcLoad);
    rom.setCode(0xEF46, md::Code().jsr(funcLoadAddr));

    // "Falling" map transition position storage injection
//    md::Code funStoreMapEntrancePositionFall;
//    funStoreMapEntrancePositionFall.movew(addr_(0xFF5400), addr_(mapEntrancePositionStorageMemoryAddress));
//    funStoreMapEntrancePositionFall.tstw(addr_(0xFF5430));
//    funStoreMapEntrancePositionFall.rts();
//    uint32_t funcStoreMapEntrancePositionFallAddr = rom.injectCode(funStoreMapEntrancePositionFall);
//    rom.setCode(0x6368, md::Code().jsr(funcStoreMapEntrancePositionFallAddr));
}

void alterFahlChallenge(md::ROM& rom)
{
    // Neutralize mid-challenge proposals for money
    rom.setCode(0x12D52, md::Code().nop(24));

    // When beating an enemy, increment the "beaten" counter by 4 instead of 1
    rom.setByte(0x12D4C, 0x58);
    
    // Set the end of the challenge at 0x30 instead of 0x32
    rom.setByte(0x12D87, 0x30);
}

void handleArmorUpgrades(md::ROM& rom)
{
    // --------------- Alter item in D0 register function ---------------
    md::Code funcAlterItemInD0;

    // Check if item ID is between 09 and 0C (armors). If not, branch to return.
    funcAlterItemInD0.cmpib(ITEM_STEEL_BREAST, reg_D0);
    funcAlterItemInD0.blt(13);
    funcAlterItemInD0.cmpib(ITEM_HYPER_BREAST, reg_D0);
    funcAlterItemInD0.bgt(11);

    // By default, put Hyper breast as given armor
    funcAlterItemInD0.movew(ITEM_HYPER_BREAST, reg_D0);

    // If Shell breast is not owned, put Shell breast
    funcAlterItemInD0.btst(0x05, addr_(0xFF1045));
    funcAlterItemInD0.bne(2);
    funcAlterItemInD0.movew(ITEM_SHELL_BREAST, reg_D0);

    // If Chrome breast is not owned, put Chrome breast
    funcAlterItemInD0.btst(0x01, addr_(0xFF1045));
    funcAlterItemInD0.bne(2);
    funcAlterItemInD0.movew(ITEM_CHROME_BREAST, reg_D0);

    // If Steel breast is not owned, put Steel breast
    funcAlterItemInD0.btst(0x05, addr_(0xFF1044));
    funcAlterItemInD0.bne(2);
    funcAlterItemInD0.movew(ITEM_STEEL_BREAST, reg_D0);

    funcAlterItemInD0.rts();

    uint32_t funcAlterItemInD0Addr = rom.injectCode(funcAlterItemInD0);


    // --------------- Change item in reward box function ---------------
    md::Code funcChangeItemRewardBox;

    funcChangeItemRewardBox.jsr(funcAlterItemInD0Addr);
    funcChangeItemRewardBox.movew(reg_D0, addr_(0xFF1196));
    funcChangeItemRewardBox.rts();

    uint32_t funcChangeItemRewardBoxAddr = rom.injectCode(funcChangeItemRewardBox);

    // --------------- Change item given by taking item on ground function ---------------
    md::Code funcAlterItemGivenByGroundSource;

    funcAlterItemGivenByGroundSource.movemToStack({ reg_D7 }, { reg_A0 }); // movem D7,A0 -(A7)	(48E7 0180)

    funcAlterItemGivenByGroundSource.cmpib(ITEM_HYPER_BREAST, reg_D0);
    funcAlterItemGivenByGroundSource.bgt(9); // to movem
    funcAlterItemGivenByGroundSource.cmpib(ITEM_STEEL_BREAST, reg_D0);
    funcAlterItemGivenByGroundSource.blt(7);  // to movem

    funcAlterItemGivenByGroundSource.jsr(funcAlterItemInD0Addr);
    funcAlterItemGivenByGroundSource.moveb(addr_(reg_A5, 0x3B), reg_D7);  // move ($3B,A5), D7	(1E2D 003B)
    funcAlterItemGivenByGroundSource.subib(0xC9, reg_D7);
    funcAlterItemGivenByGroundSource.cmpa(lval_(0xFF5400), reg_A5);
    funcAlterItemGivenByGroundSource.blt(2);    // to movem
    funcAlterItemGivenByGroundSource.bset(reg_D7, addr_(0xFF103F)); // set a flag when an armor is taken on the ground for it to disappear afterwards

    funcAlterItemGivenByGroundSource.movemFromStack({ reg_D7 }, { reg_A0 }); // movem (A7)+, D7,A0	(4CDF 0180)
    funcAlterItemGivenByGroundSource.lea(0xFF1040, reg_A0);
    funcAlterItemGivenByGroundSource.rts();

    uint32_t funcAlterItemGivenByGroundSourceAddr = rom.injectCode(funcAlterItemGivenByGroundSource);

    // --------------- Change visible item for items on ground function ---------------
    md::Code funcAlterItemVisibleForGroundSource;

    funcAlterItemVisibleForGroundSource.movemToStack({ reg_D7 }, { reg_A0 });  // movem D7,A0 -(A7)

    funcAlterItemVisibleForGroundSource.subib(0xC0, reg_D0);
    funcAlterItemVisibleForGroundSource.cmpib(ITEM_HYPER_BREAST, reg_D0);
    funcAlterItemVisibleForGroundSource.bgt(10); // to move D0 in item slot
    funcAlterItemVisibleForGroundSource.cmpib(ITEM_STEEL_BREAST, reg_D0);
    funcAlterItemVisibleForGroundSource.blt(8); // to move D0 in item slot
    funcAlterItemVisibleForGroundSource.moveb(reg_D0, reg_D7);
    funcAlterItemVisibleForGroundSource.subib(ITEM_STEEL_BREAST, reg_D7);
    funcAlterItemVisibleForGroundSource.btst(reg_D7, addr_(0xFF103F));
    funcAlterItemVisibleForGroundSource.bne(3);
    // Item was not already taken, alter the armor inside
    funcAlterItemVisibleForGroundSource.jsr(funcAlterItemInD0Addr);
    funcAlterItemVisibleForGroundSource.bra(2);
    // Item was already taken, remove it by filling it with an empty item
    funcAlterItemVisibleForGroundSource.movew(ITEM_NONE, reg_D0);
    funcAlterItemVisibleForGroundSource.moveb(reg_D0, addr_(reg_A1, 0x36)); // move D0, ($36,A1) (1340 0036)
    funcAlterItemVisibleForGroundSource.movemFromStack({ reg_D7 }, { reg_A0 }); // movem (A7)+, D7,A0	(4CDF 0180)
    funcAlterItemVisibleForGroundSource.rts();

    uint32_t funcAlterItemVisibleForGroundSourceAddr = rom.injectCode(funcAlterItemVisibleForGroundSource);

    // --------------- Hooks ---------------
    // In 'chest reward' function, replace the item ID move by the injected function
    rom.setCode(0x0070BE, md::Code().jsr(funcChangeItemRewardBoxAddr));

    // In 'NPC reward' function, replace the item ID move by the injected function
    rom.setCode(0x028DD8, md::Code().jsr(funcChangeItemRewardBoxAddr));

    // In 'item on ground reward' function, replace the item ID move by the injected function
    rom.setWord(0x024ADC, 0x3002); // put the move D2,D0 before the jsr because it helps us while changing nothing to the usual logic
    rom.setCode(0x024ADE, md::Code().jsr(funcChangeItemRewardBoxAddr));

    // Replace 2928C lea (41F9 00FF1040) by a jsr to injected function
    rom.setCode(0x02928C, md::Code().jsr(funcAlterItemGivenByGroundSourceAddr));

    // Replace 1963C - 19644 (0400 00C0 ; 1340 0036) by a jsr to a replacement function
    rom.setCode(0x01963C, md::Code().jsr(funcAlterItemVisibleForGroundSourceAddr).nop());
}


///////////////////////////////////////////////////////////////////////////////////
//       Item check changes
///////////////////////////////////////////////////////////////////////////////////

void fixAxeMagicCheck(md::ROM& rom)
{
    // Changes the Axe Magic check when slashing a tree from bit 0 of flag 1003 to "Axe Magic owned"
    // 0x16262:
        // Before:	[0839] btst 0 in FF1003
        // After:	[0839] btst 5 in FF104B
    rom.setWord(0x016264, 0x0005);
    rom.setWord(0x016268, 0x104B);
}

void fixSafetyPassCheck(md::ROM& rom)
{
    // Change Mercator door opened check from bit 7 of flag 1004 to "Safety Pass owned"
    // 0x004FF8:
        // Before:	04 0F (0F & 7 = 7 -----> bit 7 of FF1004)
        // After:	59 0D (0D & 7 = 5 -----> bit 5 of FF1059)
    rom.setWord(0x004FF8, 0x590D);
}

void fixArmletCheck(md::ROM& rom)
{
    // Change Armlet check from bit 6 of flag 1014 to "Armlet owned".
    // This one was tricky because there are TWO checks to change (the one to remove the "repelled" textbox + tornado, 
    // and another one to remove the invisible wall added in US version), and any of those checks remove the armlet from inventory
    // on completion.

    // Actually changed the flag which is check for both triggers
    // 0x09C803:
        // Before:	14 06 (bit 6 of FF1014)
        // After:	4F 05 (bit 5 of FF104F)
    rom.setWord(0x09C803, 0x4F05);
    // 0x09C7F3: same as above
    rom.setWord(0x09C7F3, 0x4F05);
    
    // Prevent the game from removing the armlet from the inventory
    // 0x013A80: put a RTS instead of the armlet removal and all (not exactly sure why it works perfectly, but it does)
        // Before:  4E F9
        // After:	4E 75 (rts)
    rom.setCode(0x013A8A, md::Code().rts());
}

void fixSunstoneCheck(md::ROM& rom)
{
    // Change Sunstone check for repairing the lighthouse from bit 2 of flag 1026 to "Susntone owned"
    // 0x09D091:
        // Before:	26 02 (bit 2 of FF1026)
        // After:	4F 01 (bit 1 of FF104F)
    rom.setWord(0x09D091, 0x4F01);
}

void fixDogTalkingCheck(md::ROM& rom)
{
    // Change doggo talking check from bit 4 of flag 1024 to "Einstein Whistle owned"
    // 0x0253C0:
        // Before:	01 24 (0124 >> 3 = 24 and 0124 & 7 = 04 -----> bit 4 of FF1024)
        // After:	02 81 (0281 >> 3 = 50 and 0281 & 7 = 01 -----> bit 1 of FF1050)
    rom.setWord(0x0253C0, 0x0281);
}

void alterMercatorSecondaryShopCheck(md::ROM& rom)
{
    // Change the Mercator secondary shop check so that it sells item as long as you own Buyer's Card
    // 0x00A574:
        // Before:	2A 04 (bit 4 of FF102A)
        // After:	4C 05 (bit 5 of FF104C)
    rom.setWord(0x00A574, 0x4C05);
}

void addJewelsCheckForTeleporterToKazalt(md::ROM& rom, const RandomizerOptions& options)
{
    // ----------- Rejection textbox handling ------------
    md::Code funcRejectKazaltTeleporter;

    funcRejectKazaltTeleporter.jsr(0x22EE8); // open textbox
    funcRejectKazaltTeleporter.movew(0x22, reg_D0);
    funcRejectKazaltTeleporter.jsr(0x28FD8); // display text 
    funcRejectKazaltTeleporter.jsr(0x22EA0); // close textbox
    funcRejectKazaltTeleporter.rts();

    uint32_t addrRejectKazaltTeleport = rom.injectCode(funcRejectKazaltTeleporter);

    // ----------- Jewel checks handling ------------
    md::Code procHandleJewelsCheck;

    if(options.getJewelCount() > MAX_INDIVIDUAL_JEWELS)
    {
        procHandleJewelsCheck.movemToStack({reg_D1},{});
        procHandleJewelsCheck.moveb(addr_(0xFF1054), reg_D1);
        procHandleJewelsCheck.andib(0x0F, reg_D1);
        procHandleJewelsCheck.cmpib(options.getJewelCount(), reg_D1); // Test if red jewel is owned
        procHandleJewelsCheck.movemFromStack({reg_D1},{});
        procHandleJewelsCheck.bgt(3);
            procHandleJewelsCheck.jsr(addrRejectKazaltTeleport);
            procHandleJewelsCheck.rts();
    }
    else
    {
        if(options.getJewelCount() >= 1)
        {
            procHandleJewelsCheck.btst(0x1, addr_(0xFF1054)); // Test if red jewel is owned
            procHandleJewelsCheck.bne(3);
                procHandleJewelsCheck.jsr(addrRejectKazaltTeleport);
                procHandleJewelsCheck.rts();
        }
        if(options.getJewelCount() >= 2)
        {
            procHandleJewelsCheck.btst(0x1, addr_(0xFF1055)); // Test if purple jewel is owned
            procHandleJewelsCheck.bne(3);
                procHandleJewelsCheck.jsr(addrRejectKazaltTeleport);
                procHandleJewelsCheck.rts();
        }
        if(options.getJewelCount() >= 3)
        {
            procHandleJewelsCheck.btst(0x1, addr_(0xFF105A)); // Test if green jewel is owned
            procHandleJewelsCheck.bne(3);
                procHandleJewelsCheck.jsr(addrRejectKazaltTeleport);
                procHandleJewelsCheck.rts();
        }
        if(options.getJewelCount() >= 4)
        {
            procHandleJewelsCheck.btst(0x5, addr_(0xFF1050)); // Test if blue jewel is owned
            procHandleJewelsCheck.bne(3);
                procHandleJewelsCheck.jsr(addrRejectKazaltTeleport);
                procHandleJewelsCheck.rts();
        }
        if(options.getJewelCount() >= 5)
        {
            procHandleJewelsCheck.btst(0x1, addr_(0xFF1051)); // Test if yellow jewel is owned
            procHandleJewelsCheck.bne(3);
                procHandleJewelsCheck.jsr(addrRejectKazaltTeleport);
                procHandleJewelsCheck.rts();
        }
    }
    procHandleJewelsCheck.moveq(0x7, reg_D0);
    procHandleJewelsCheck.jsr(0xE110);  // "func_teleport_kazalt"
    procHandleJewelsCheck.jmp(0x62FA);

    uint32_t procHandleJewelsAddr = rom.injectCode(procHandleJewelsCheck);

    // This adds the purple & red jewel as a requirement for the Kazalt teleporter to work correctly
    rom.setCode(0x62F4, md::Code().jmp(procHandleJewelsAddr));
}

void makeSwordOfGaiaWorkInVolcano(md::ROM& rom)
{
    // Add the ability to also trigger the volcano using the Sword of Gaia instead of only Statue of Gaia
    md::Code procTriggerVolcano;

    procTriggerVolcano.cmpiw(0x20A, addr_(0xFF1204));
    procTriggerVolcano.bne(4);
        procTriggerVolcano.bset(0x2, addr_(0xFF1027));
        procTriggerVolcano.jsr(0x16712);
        procTriggerVolcano.rts();
    procTriggerVolcano.jmp(0x16128);

    uint32_t procAddr = rom.injectCode(procTriggerVolcano);

    rom.setCode(0x1611E, md::Code().jmp(procAddr));
}

void fixReverseGreenmazeFountainSoftlock(md::ROM& rom)
{
    // Pressing the button behind the locked door now triggers the flag "Visited Greenmaze Crossroads map"
    rom.setByte(0x9C66C, 0xF1);
    rom.setByte(0x9C66D, 0x00);

    // Change the flag checked for lowering fountain to "Visited Greenmaze Crossroads map"
    rom.setByte(0x500C, 0xF1);
    rom.setByte(0x500D, 0x08);
}

void alterCasinoTicketHandling(md::ROM& rom)
{
    // Remove ticket consumption on dialogue with the casino "bouncer"
    rom.setCode(0x277A8, md::Code().nop(2));

    // Remove Arthur giving his item another time if casino ticket is not owned
    rom.setCode(0x26BBA, md::Code().nop(5));
}

void addDoorForReverseSafetyPass(md::ROM& rom)
{
    // Alter map variants so that we are in map 0x27A while safety pass is not owned
    rom.setWord(0xA50E, 0x5905); // Byte 1050 bit 5 is required to trigger map variant
    rom.setWord(0xA514, 0x5905); // Byte 1050 bit 5 is required to trigger map variant

    // Modify entities in map variant 0x27A to replace two NPCs by a door blocking the way out of Mercator
    rom.setBytes(0x2153A, { 0x70, 0x2A, 0x02, 0x00, 0x00, 0x67, 0x01, 0x00, 0x70, 0x2C, 0x02, 0x00, 0x00, 0x67, 0x01, 0x00 });
}

void makeRyumaMayorSaveable(md::ROM& rom)
{
    // --------------- Fixes for thieves hideout treasure room ---------------
    // Disable the cutscene when opening vanilla lithograph chest
    rom.setCode(0x136BE, md::Code().rts());

    // Disable Friday blocker in the treasure room by removing last entity from the map
    rom.setWord(0x9BA62, 0xFEFE); // Why does that even work? I don't know...

    // Set the "Mayor freed" flag to byte 10D6, bit 4 (which happens to be the "Visited treasure room with mayor variant" flag)
    rom.setWord(0xA3F4, 0xD604);

    // Remove the "remove all NPCs on flag set" trigger in treasure room by putting it to an impossible flag
    rom.setByte(0x1A9C0, 0x01);

    // Inject custom code "on map enter" to set the flag when it is convenient to do so
    md::Code onMapEnterFunction;
    onMapEnterFunction.lea(0xFF10C0, reg_A0); // Do the instruction that was replaced the hook call
    // When entering the cavern where we save Ryuma's mayor, set the flag "mayor is saved"
    onMapEnterFunction.cmpib(0xE0, reg_D0);
    onMapEnterFunction.bne(2);
        onMapEnterFunction.bset(0x01, addr_(0xFF1004));
    onMapEnterFunction.rts();

    uint32_t onMapEnterFunctionAddr = rom.injectCode(onMapEnterFunction);
    rom.setCode(0x2952, md::Code().jsr(onMapEnterFunctionAddr));

    // --------------- Fixes for Ryuma's mayor reward ---------------
    // Change the second reward from "fixed 100 golds" to "item with ID located at 0x2837F"
    rom.setLong(0x2837E, 0x000017E8);

    // Remove the "I think we need an exorcism" dialogue for the mayor when progression flags are much further in the game
    rom.setWord(0x2648E, 0xF908); // CheckFlagAndDisplayMessage
    rom.setWord(0x26490, 0x0023); // on bit 3 of flag FF1004
    rom.setWord(0x26492, 0x1801); // Script ID for post-reward dialogue
    rom.setWord(0x26494, 0x17FF); // Script ID for reward cutscene
    rom.setCode(0x26496, md::Code().rts());
    // Clear out the rest
    rom.setLong(0x26498, 0xFFFFFFFF);
}

///////////////////////////////////////////////////////////////////////////////////
//       Map check changes
///////////////////////////////////////////////////////////////////////////////////

void alterWaterfallShrineSecretStairsCheck(md::ROM& rom)
{
    // Change Waterfall Shrine entrance check from "Talked to Prospero" to "What a noisy boy!", removing the need
    // of talking to Prospero (which we couldn't do anyway because of the story flags).

    // 0x005014:
        // Before:	00 08 (bit 0 of FF1000)
        // After:	02 09 (bit 1 of FF1002)
    rom.setWord(0x005014, 0x0209);
}

void makeFallingRibbonAlwaysThere(md::ROM& rom)
{
    // The "falling ribbon" item source is pretty dependant from the scenario to happen. In the original game,
    // the timeframe to get it is really tight. We try to get rid of any conditions here, apart from checking
    // if item has already been obtained.

    // 0x00A466:
        // Before:  20 05 (bit 5 of flag 1020)
        // After:   3F 07 (bit 7 of flag 103F - never true)
    rom.setWord(0x00A466, 0x3F07);

    // 0x01A724:
        // Before:	16 22 (bit 1 of flag 1016)
        // After:	3F E2 (bit 7 of flag 103F - never true)
    rom.setWord(0x1A724, 0x3FE2);

    // Change falling item position from 1F to 20 to ensure it is taken by Nigel whatever its original position is
    rom.setByte(0x09C59E, 0x20);

    // Remove the servant guarding the door, setting her position to 00 00
    rom.setWord(0x01BFCA, 0x0000);
}

void alterKingNolesCaveTeleporterCheck(md::ROM& rom)
{
    // Change the flag checked for teleporter appearance from "saw the duke Kazalt cutscene" to "has visited four white golems room in King Nole's Cave"
    // 0x0050A0:
        // Before:	27 09 (bit 1 of flag 1027)
        // After:	D0 09 (bit 1 of flag 10D0)
    rom.setWord(0x0050A0, 0xD009);
    // 0x00509C:
        // Before:	27 11 (bit 1 of flag 1027)
        // After:	D0 11 (bit 1 of flag 10D0)
    rom.setWord(0x00509C, 0xD011);

    // We need to inject a procedure checking "is D0 equal to FF" to replace the "bmi" previously used which was preventing
    // from checking flags above 0x80 (the one we need to check is 0xD0).

    md::Code procImproveFlagCheck;

    procImproveFlagCheck.moveb(addr_(reg_A0, 0x2), reg_D0);     // 1028 0002
    procImproveFlagCheck.cmpib(0xFF, reg_D0);
    procImproveFlagCheck.bne(2);
    procImproveFlagCheck.jmp(0x4E2E);
    procImproveFlagCheck.jmp(0x4E20);

    uint32_t procAddr = rom.injectCode(procImproveFlagCheck);

    // Replace the (move.b, bmi, ext.w) by a jmp to the injected procedure
    rom.setCode(0x004E18, md::Code().clrw(reg_D0).jmp(procAddr));
}

void makeMercatorDocksShopAlwaysOpen(md::ROM& rom)
{
    // 0x01AA26:
        // Before:	0284 2A A2 (in map 284, check bit 5 of flag 102A)
        // After:	0000 5F E2 (in map 0, check bit 7 of flag 105F - never true)
    rom.setWord(0x01AA26, 0x0000);
    rom.setWord(0x01AA28, 0x5FE2);
}

void makeArthurAlwaysPresentInThroneRoom(md::ROM& rom)
{
    // Change the Arthur check giving casino tickets for him to be always here, instead of only after Lake Shrine

    // 0x01A904: 
        // Before:  AAA0 (bit 5 of flag 2A, affecting entity 0) 
        // After:   8000 (bit 0 of flag 00, affecting entity 0)
    rom.setWord(0x01A904, 0x8000);

    // 0x01A908:
        // Before:	AAA1 (bit 5 of flag 2A, affecting entity 1) 
        // After:	8001 (bit 0 of flag 00, affecting entity 1)
    rom.setWord(0x01A908, 0x8001);
}

void fixCryptBehavior(md::ROM& rom)
{
    // 1) Remove the check "if shadow mummy was beaten, raft mummy never appears again"
    // 0x019DF6:
        // Before:	0839 0006 00FF1014 (btst bit 6 in FF1014) ; 66 14 (bne $19E14)
        // After:	4EB9 00019E14 (jsr $19E14; 4E71 4E71 (nop nop)
    rom.setCode(0x19DF6, md::Code().nop(5));

    // 2) Change the room exit check and shadow mummy appearance from "if armlet is owned" to "chest was opened"
    // 0x0117E8:
        // Before:	103C 001F ; 4EB9 00022ED0 ; 4A41 ; 6B00 F75C (bmi $10F52)
        // After:	0839 0002 00FF1097 (btst 2 FF1097)	; 6700 F75C (bne $10F52)
    md::Code injectChangeCryptExitCheck;
    injectChangeCryptExitCheck.btst(0x2, addr_(0xFF1097));
    injectChangeCryptExitCheck.nop(2);
    injectChangeCryptExitCheck.beq(); // beq $10F52
    rom.setCode(0x117E8, injectChangeCryptExitCheck);
}

void fixKingNolesLabyrinthRafts(md::ROM& rom)
{
    // Change the rafts logic so we can take them several times in a row, preventing from getting softlocked by missing chests
    // The trick here is to use flag 1001 (which resets on every map change) to correctly end the cutscene while discarding the "raft already taken" state 
    // as early as the player moves to another map.
    rom.setWord(0x09E031, 0x0100);
    rom.setWord(0x09E034, 0x0100);
    rom.setWord(0x09E04E, 0x0100);
    rom.setWord(0x09E051, 0x0100);
}

void removeLogsRoomExitCheck(md::ROM& rom)
{
    // Remove logs check
    rom.setCode(0x011EC4, md::Code().bra());
}

void fixMirFightAfterLakeShrine(md::ROM& rom)
{
    // In the original game, coming back to Mir room after Lake Shrine would softlock you because Mir
    // would not be there. This check is removed to prevent any softlock and allow fighting Mir after having
    // done Lake Shrine.

    // 0x01AA22:
        // Before:	0310 2A A2 (in map 310, check bit 5 of flag 102A)
        // After:	0000 5F E2 (in map 0, check bit 7 of flag 105F - never true)
    rom.setWord(0x01AA22, 0x0000);
    rom.setWord(0x01AA24, 0x5FE2);
}

void fixMirTowerPriestRoomItems(md::ROM& rom)
{
    // Remove the "shop/church" flag on the priest room of Mir Tower to make its items on ground work everytime
    // 0x024E5A:
        // Before:	0307
        // After:	7F7F
    rom.setWord(0x024E5A, 0x7F7F);
}


///////////////////////////////////////////////////////////////////////////////////
//       Map content changes
///////////////////////////////////////////////////////////////////////////////////

void removeMercatorCastleBackdoorGuard(md::ROM& rom)
{
    // There is a guard staying in front of the Mercator castle backdoor to prevent you from using
    // Mir Tower keys on it. He appears when Crypt is finished and disappears when Mir Tower is finished,
    // but we actually never want him to be there, so we delete him from existence by moving him away from the map.

    // 0x0215A6:
        // Before:	93 9D
        // After:	00 00
    rom.setWord(0x0215A6, 0x0000);
}

void removeSailorInDarkPort(md::ROM& rom)
{
    // There is a sailor NPC in the "dark" version of Mercator port who responds badly to story triggers, allowing us to sail to Verla
    // even without having repaired the lighthouse. To prevent this from being exploited, we removed him altogether.

    // 0x021646:
        // Before:	23 EC
        // After:	00 00
    rom.setWord(0x021646, 0x0000);
}

void fixFaraLifestockChest(md::ROM& rom)
{
    // Make it so Lifestock chest near Fara in Swamp Shrine appears again when going back into the room afterwards, preventing any softlock there.

    // --------- Function to remove all entities but the chest when coming back in the room ---------
    md::Code funcRemoveAllEntitiesButChestInFaraRoom;

    funcRemoveAllEntitiesButChestInFaraRoom.movemToStack({ reg_D0_D7 }, { reg_A0_A6 });
    funcRemoveAllEntitiesButChestInFaraRoom.lea(0xFF5480, reg_A0);
    funcRemoveAllEntitiesButChestInFaraRoom.moveq(0xD, reg_D0);
    funcRemoveAllEntitiesButChestInFaraRoom.label("loop_remove_entities");
    funcRemoveAllEntitiesButChestInFaraRoom.movew(0x7F7F, addr_(reg_A0));
    funcRemoveAllEntitiesButChestInFaraRoom.adda(0x80, reg_A0);
    funcRemoveAllEntitiesButChestInFaraRoom.dbra(reg_D0, "loop_remove_entities");
    funcRemoveAllEntitiesButChestInFaraRoom.movemFromStack({ reg_D0_D7 }, { reg_A0_A6 });
    funcRemoveAllEntitiesButChestInFaraRoom.rts();

    uint32_t funcAddr = rom.injectCode(funcRemoveAllEntitiesButChestInFaraRoom);

    // Call the injected function
    rom.setCode(0x019BE0, md::Code().jsr(funcAddr).nop());

    // --------- Moving the chest to the ground ---------
    rom.setWord(0x01BF6C, 0x1A93);
    rom.setWord(0x01BF6E, 0x0000);
    rom.setWord(0x01BF70, 0x0012);
    rom.setWord(0x01BF72, 0x0400);
}

void replaceLumberjackByChest(md::ROM& rom)
{
    // Set base index for chests in map to "1A" instead of "A8" to have room for a second chest in the map
    for (uint32_t addr = 0x9E9BA; addr <= 0x9E9BE; ++addr)
        rom.setByte(addr, 0x1A);

    // Transform the lumberjack into a chest
    rom.setWord(0x020B9C, 0xD5B1);  // First word: position, orientation and palette (5571 => D5B1)
    rom.setWord(0x020B9E, 0x0000);  // Second word: ??? (2200 => 0000)
    rom.setWord(0x020BA0, 0x0012);  // Third word: type (0550 = Lumberjack NPC => 0012 = chest)
    rom.setWord(0x020BA2, 0x8000);  // Fourth word: behavior (00C1 => 8000 works for some reason)
}

void replaceSickMerchantByChest(md::ROM& rom)
{
    // Neutralize map entrance triggers for both the shop and the backroom to remove the "sidequest complete" check.
    // Either we forced them to be always true or always false
    rom.setWord(0x0050B4, 0x0008);  // Before: 0x2A0C (bit 4 of 102A) | After: 0x0008 (bit 0 of 1000 - always true)
    rom.setWord(0x00A568, 0x3F07);	// Before: 0x2A04 (bit 4 of 102A) | After: 0x3F07 (bit 7 of 103F - always false)
    rom.setWord(0x00A56E, 0x3F07);	// Before: 0x2A03 (bit 3 of 102A) | After: 0x3F07 (bit 7 of 103F - always false)
    rom.setWord(0x01A6F8, 0x3FE0);	// Before: 0x2A80 (bit 4 of 102A) | After: 0x3FE0 (bit 7 of 103F - always false)

    // Set the index for added chest in map to "0E" instead of "C2"
    rom.setByte(0x09EA48, 0x0E);

    // Transform the sick merchant into a chest
    rom.setWord(0x021D0E, 0xCE92);  // First word: position, orientation and palette (CE16 => CE92)
    rom.setWord(0x021D10, 0x0000);  // Second word: ??? (3000 => 0000)
    rom.setWord(0x021D12, 0x0012);  // Third word: type (006D = Lumberjack NPC => 0012 = chest)
//	rom.setWord(0x021D14, 0x0000); 

    // Move the kid to hide the fact that the bed looks broken af
    rom.setWord(0x021D16, 0x5055);
}

void replaceFaraInElderHouseByChest(md::ROM& rom)
{
    // Neutralize a map specific trigger which broke chests inside it
    // 0x019C82:
        // Before:	0x0251 (map ID)
        // After:	0xFFFF 
    rom.setWord(0x019C82, 0xFFFF);

    // Move the elder to his right position
    rom.setWord(0x020FA2, 0x1113);

    // Replace Fara entity by a chest
    rom.setWord(0x020FAA, 0x134F);
    rom.setWord(0x020FAC, 0x0000);
    rom.setWord(0x020FAE, 0x0012);
    rom.setWord(0x020FB0, 0x0000);

    // Remove bed wakeup cutscene
    rom.setWord(0x020FB2, 0x7F7F);
    rom.setWord(0x020FB4, 0x0000);
    rom.setWord(0x020FB6, 0x0000);
    rom.setWord(0x020FB8, 0x0000);

    // Set map base chest index to 0x17
    rom.setByte(0x09E9DF, 0x17);
}



///////////////////////////////////////////////////////////////////////////////////
//       Original game bugs & glitches fixes
///////////////////////////////////////////////////////////////////////////////////

void fixArmletSkip(md::ROM& rom)
{
    // Fix armlet skip by putting the tornado way higher, preventing any kind of buffer-jumping on it
    // 0x02030C:
        // Before:  0x82 (Tornado pos Y = 20)
        // After:   0x85 (Tornado pos Y = 50)
    rom.setByte(0x02030C, 0x85);
}

void fixTreeCuttingGlitch(md::ROM& rom)
{
    // Inject a new function which fixes the money value check on an enemy when it is killed, causing the tree glitch to be possible
    md::Code funcFixTreeCuttingGlitch;
 
    funcFixTreeCuttingGlitch.tstb(addr_(reg_A5, 0x36));  // tst.b ($36,A5) [4A2D 0036]
    funcFixTreeCuttingGlitch.beq(4);
        // Only allow the "killable because holding money" check if the enemy is not a tree
        funcFixTreeCuttingGlitch.cmpiw(0x126, addr_(reg_A5, 0xA));
        funcFixTreeCuttingGlitch.beq(2);
            funcFixTreeCuttingGlitch.jmp(0x16284);
    funcFixTreeCuttingGlitch.rts();

    uint32_t funcAddr = rom.injectCode(funcFixTreeCuttingGlitch);

    // Call the injected function when killing an enemy
    rom.setCode(0x01625C, md::Code().jsr(funcAddr));
}



///////////////////////////////////////////////////////////////////////////////////
//       Miscellaneous
///////////////////////////////////////////////////////////////////////////////////

void deactivateRegionCheck(md::ROM& rom)
{
    // Before : jsr $A0A0C | After : nop nop nop
    rom.setCode(0x506, md::Code().nop(3));
}

void changeHUDColor(md::ROM& rom, const RandomizerOptions& options)
{
    // 0x824 is the default purple color from the original game
    uint16_t color = 0x0824;

    std::string hudColor = options.getHUDColor();
    Tools::toLower(hudColor);

    if (hudColor == "red")              color = 0x228;
    else if (hudColor == "darkred")     color = 0x226;
    else if (hudColor == "green")       color = 0x262;
    else if (hudColor == "blue")        color = 0x842;
    else if (hudColor == "brown")       color = 0x248;
    else if (hudColor == "darkpurple")  color = 0x424;
    else if (hudColor == "darkgray")    color = 0x222;
    else if (hudColor == "gray")        color = 0x444;
    else if (hudColor == "lightgray")   color = 0x666;

    rom.setWord(0xF6D0, color);
    rom.setWord(0xFB36, color);
    rom.setWord(0x903C, color);
//    rom.setWord(0x9020, color);
}

void alterLanternHandling(md::ROM& rom)
{
//    rom.storeAddress("data_dark_rooms", 0x8800);

    // ----------------------------------------
    // Function to darken the color palette currently used to draw the map
    // Input :  D0 = AND mask to apply to palette

    md::Code funcAlterPalette; // 0x0402 to darken

    funcAlterPalette.movemToStack({ reg_D1, reg_D2 }, { reg_A0 });
    funcAlterPalette.lea(0xFF0080, reg_A0);
    funcAlterPalette.movew(0x20, reg_D1);
    funcAlterPalette.label("loop");
        funcAlterPalette.movew(addr_(reg_A0), reg_D2);
        funcAlterPalette.andw(reg_D0, reg_D2);
        funcAlterPalette.movew(reg_D2, addr_(reg_A0));
        funcAlterPalette.adda(0x2, reg_A0);
        funcAlterPalette.dbra(reg_D1, "loop");
    funcAlterPalette.movemFromStack({ reg_D1, reg_D2 }, { reg_A0 });
    funcAlterPalette.rts();

    uint32_t funcAlterPaletteAddr = rom.injectCode(funcAlterPalette);

    // ----------------------------------------
    // Function to check if the current room is supposed to be dark, and process differently 
    // depending on whether or not we own the lantern
    md::Code funcLanternCheck;

    funcLanternCheck.lea(rom.getStoredAddress("data_dark_rooms"), reg_A0);
    funcLanternCheck.label("loop_start");
    funcLanternCheck.movew(addr_(reg_A0), reg_D0);
    funcLanternCheck.bmi(14);
        funcLanternCheck.cmpw(addr_(0xFF1204), reg_D0);
        funcLanternCheck.bne(10);
            // We are in a dark room
            funcLanternCheck.movemToStack({ reg_D0 }, {});
            funcLanternCheck.btst(0x1, addr_(0xFF104D));
            funcLanternCheck.bne(3);
                // Dark room with no lantern ===> darken the palette
                funcLanternCheck.movew(0x0000, reg_D0);
                funcLanternCheck.bra(2);

                // Dark room with lantern ===> use lantern palette
                funcLanternCheck.movew(0x0CCC, reg_D0);
            funcLanternCheck.jsr(funcAlterPaletteAddr);
            funcLanternCheck.movemFromStack({ reg_D0 }, {});
            funcLanternCheck.rts();
        funcLanternCheck.addql(0x2, reg_A0);
        funcLanternCheck.bra("loop_start");
    funcLanternCheck.clrb(reg_D7);
    funcLanternCheck.rts();

    rom.setCode(0x87BE, funcLanternCheck);

    // ----------------------------------------
    // Function to change the palette used on map transition (already exist in OG, slightly modified)
    md::Code funcChangeMapPalette;

    funcChangeMapPalette.cmpb(addr_(0xFF112F), reg_D4);
    funcChangeMapPalette.beq(12);
        funcChangeMapPalette.moveb(reg_D4, addr_(0xFF112F));
        funcChangeMapPalette.lea(0x11C926, reg_A0);
        funcChangeMapPalette.mulu(bval_(0x1A), reg_D4);
        funcChangeMapPalette.adda(reg_D4, reg_A0);
        funcChangeMapPalette.lea(0xFF0084, reg_A1);
        funcChangeMapPalette.movew(0x000C, reg_D0);
        funcChangeMapPalette.jsr(0x96A);
        funcChangeMapPalette.clrw(addr_(0xFF0080));
        funcChangeMapPalette.movew(0x0CCC, addr_(0xFF0082));
        funcChangeMapPalette.clrw(addr_(0xFF009E));
        funcChangeMapPalette.jsr(0x87BE); // TOOD: This call could be moved to the function which calls this one on menu exit
    funcChangeMapPalette.rts();

    rom.setCode(0x2D64, funcChangeMapPalette);
    
    // ----------------------------------------
    // Pseudo-function used to extend the global palette init function, used to call the lantern check
    // after initing all palettes (both map & entities)
    md::Code funcExtendPaletteInit;

    funcExtendPaletteInit.movew(0x0CCC, addr_(0xFF00A2));
    funcExtendPaletteInit.jsr(0x1A4414);
    funcExtendPaletteInit.jsr(0x87BE);
    funcExtendPaletteInit.rts();

    rom.setLong(0x19522, rom.injectCode(funcExtendPaletteInit));

    // ----------------------------------------
    // Replace the "dark room" palette from King Nole's Labyrinth by the lit room palette
    constexpr uint32_t litRoomPaletteAddr = 0x11CD1C;
    constexpr uint32_t darkRoomPaletteAddr = 0x11CD36;
    for (uint8_t i = 0; i < 0x1A; ++i)
        rom.setByte(darkRoomPaletteAddr + i, rom.getByte(litRoomPaletteAddr + i));
}

void alterGoldRewardsHandling(md::ROM& rom)
{
    // In the original game, only 3 item IDs are reserved for gold rewards (3A, 3B, 3C)
    // Here, we moved the table of gold rewards to the end of the ROM so that we can handle 64 rewards up to 255 golds each.
    // In the new system, all item IDs after the "empty item" one (0x40 and above) are now gold rewards.
    
    rom.setByte(0x0070DF, ITEM_GOLDS_START); // cmpi 3A, D0 >>> cmpi 40, D0
    rom.setByte(0x0070E5, ITEM_GOLDS_START); // subi 3A, D0 >>> subi 40, D0

    // ------------- Function to put gold reward value in D0 ----------------
    // Input: D0 = gold reward ID (offset from 0x40)
    // Output: D0 = gold reward value

    md::Code funcGetGoldReward;

    funcGetGoldReward.movemToStack({}, { reg_A0 });
    funcGetGoldReward.lea(rom.getStoredAddress("data_gold_values"), reg_A0);
    funcGetGoldReward.moveb(addr_(reg_A0, reg_D0, md::Size::WORD), reg_D0);  // move.b (A0, D0.w), D0 : 1030 0000
    funcGetGoldReward.movemFromStack({}, { reg_A0 });
    funcGetGoldReward.rts();

    uint32_t funcAddr = rom.injectCode(funcGetGoldReward);

    // Set the call to the injected function
    // Before:      add D0,D0   ;   move.w (PC, D0, 42), D0
    // After:       jsr to injected function
    rom.setCode(0x0070E8, md::Code().jsr(funcAddr));
}

void addTeleportFunctionToSpellBook(md::ROM& rom)
{
    uint16_t spawnX = rom.getByte(0x0027FD);
    uint16_t spawnZ = rom.getByte(0x002805);
    uint16_t spawnPosition = (spawnX << 8) + spawnZ;
    uint16_t spawnMapID = rom.getWord(0x0027F4);

    // ------------- New spell book teleport function -------------
    md::Code spellBookFunction;
    spellBookFunction.movemToStack({ reg_D0_D7 }, { reg_A0_A6 });
    spellBookFunction.movew(spawnPosition, addr_(0xFF5400));
    spellBookFunction.movew(0x0708, addr_(0xFF5402)); // Reset subtiles position
    spellBookFunction.trap(0, { 0x00, 0x4D });
    spellBookFunction.jsr(0x44C);
    spellBookFunction.movew(spawnMapID, reg_D0); // Set MapID to spawn map
    spellBookFunction.movew(0x0000, addr_(0xFF5412)); // Reset player height
    spellBookFunction.moveb(0x00, addr_(0xFF5422)); // Reset ground height
    spellBookFunction.moveb(0x00, addr_(0xFF5439)); // ^
    spellBookFunction.jsr(0x1586E);
    spellBookFunction.jsr(0x434);
    spellBookFunction.clrb(reg_D0);
    spellBookFunction.jsr(0x2824);
    spellBookFunction.jsr(0x410);
    spellBookFunction.jsr(0x8EB4);
    spellBookFunction.movemFromStack({ reg_D0_D7 }, { reg_A0_A6 });
    spellBookFunction.rts();
    rom.injectCode(spellBookFunction, "func_spell_book");

    // ------------- Extended item post use table -------------
    uint32_t newPostUseTableAddr = rom.reserveDataBlock(0x2A+6, "new_post_use_table");
    // PostUseGarlic
    rom.setLong(newPostUseTableAddr, 0x00008BE8); 
    rom.setWord(newPostUseTableAddr+4, 0x9BFF);
    newPostUseTableAddr += 6;
    // PostUseEinsteinWhistle
    rom.setLong(newPostUseTableAddr, 0x00008BF2); 
    rom.setWord(newPostUseTableAddr+4, 0xA0FF);
    newPostUseTableAddr += 6;
    // PostUsePostUseGolasEye
    rom.setLong(newPostUseTableAddr, 0x00008C7C); 
    rom.setWord(newPostUseTableAddr+4, 0xABFF);
    newPostUseTableAddr += 6;
    // PostUseIdolStone
    rom.setLong(newPostUseTableAddr, 0x00008C8E); 
    rom.setWord(newPostUseTableAddr+4, 0xB1FF);
    newPostUseTableAddr += 6;
    // PostUseKey
    rom.setLong(newPostUseTableAddr, 0x00008CB6); 
    rom.setWord(newPostUseTableAddr+4, 0xB2FF);
    newPostUseTableAddr += 6;
    // PostUseShortcake
    rom.setLong(newPostUseTableAddr, 0x00008CF6); 
    rom.setWord(newPostUseTableAddr+4, 0xB6FF);
    newPostUseTableAddr += 6;
    // PostUseSpellbook
    rom.setLong(newPostUseTableAddr, rom.getStoredAddress("func_spell_book")); 
    rom.setWord(newPostUseTableAddr+4, 0xA4FF);
    newPostUseTableAddr += 6;
    // End
    rom.setLong(newPostUseTableAddr, 0xFFFFFFFF); 
    rom.setWord(newPostUseTableAddr+4, 0xFFFF);

    // ------------- New function to read the new table -------------
    md::Code newPostUseTableLookupProc;
    newPostUseTableLookupProc.moveb(addr_(0x00FF1152), reg_D0);
    newPostUseTableLookupProc.lea(rom.getStoredAddress("new_post_use_table"), reg_A0);
    newPostUseTableLookupProc.label("newPostUseTableLookupProc_loop");
    newPostUseTableLookupProc.moveb(addr_(reg_A0, 0x04), reg_D2);
    newPostUseTableLookupProc.cmpib(0xFF, reg_D2);
    newPostUseTableLookupProc.beq(7);
    newPostUseTableLookupProc.cmpb(reg_D0, reg_D2);
    newPostUseTableLookupProc.beq(3);
    newPostUseTableLookupProc.addql(0x06, reg_A0);
    newPostUseTableLookupProc.bra("newPostUseTableLookupProc_loop");
    newPostUseTableLookupProc.movel(addr_(reg_A0), reg_A0);
    newPostUseTableLookupProc.jmp(addr_(reg_A0));
    newPostUseTableLookupProc.rts();
    uint32_t newPostUseTableLookupProcAddr = rom.injectCode(newPostUseTableLookupProc);

    // ------------- JMP to override old lookup function by the new one -------------
    md::Code postUseTableInjectorCall;
    postUseTableInjectorCall.jmp(newPostUseTableLookupProcAddr);
    rom.setCode(0x8BC8, postUseTableInjectorCall);

    // Replace "consume Spell Book" by "trigger post use effect"
    rom.setLong(0x88C6, 0x600002EA); // bra loc_8BB2 >>> Mark used item as "has post use effect"
}


void addFunctionToItemsOnUse(md::ROM& rom)
{
    addTeleportFunctionToSpellBook(rom);

    // ------------- Extended item handling function -------------

    md::Code funcExtendedItemHandling;

    funcExtendedItemHandling.cmpib(ITEM_RECORD_BOOK, reg_D0);
    funcExtendedItemHandling.bne(3);
        funcExtendedItemHandling.jsr(rom.getStoredAddress("func_save_game_record_book"));
        funcExtendedItemHandling.rts();
    funcExtendedItemHandling.cmpib(ITEM_SPELL_BOOK, reg_D0);
    funcExtendedItemHandling.bne(3);
        funcExtendedItemHandling.jsr(0xDC1C); // "func_abracadabra"
        funcExtendedItemHandling.rts();
    funcExtendedItemHandling.cmpib(ITEM_LITHOGRAPH, reg_D0);
    funcExtendedItemHandling.bne(3);
        funcExtendedItemHandling.movew(0x21, reg_D0);  // funcExtendedItemHandling.movel(rom.getStoredAddress("data_lithograph_hint_text"), addr_(0xFF0014));
        funcExtendedItemHandling.jsr(0x22E90);
    funcExtendedItemHandling.rts();
    
    uint32_t funcExtendedItemHandlingAddr = rom.injectCode(funcExtendedItemHandling);

    rom.setCode(0x00DBA8, md::Code().jsr(funcExtendedItemHandlingAddr).nop(4));

    // -------------------- Other modifications ---------------------

    // To remove the "Nothing happened..." text, the item must be put in a list which has a finite size.
    // We replace the Blue Ribbon (0x18) by the Record Book (0x23) to do so.
    rom.setByte(0x008642, 0x23);
    // Same for Lithograph (0x27) remplacing Lantern (0x1A)
    rom.setWord(0x008647, 0x6627);
}

void shortenMirCutsceneAfterLakeShrine(md::ROM& rom)
{
    // Cut the cutscene script
    rom.setWord(0x28A44, 0xE739);
}

void renameItems(md::ROM& rom, const RandomizerOptions& options)
{
    std::vector<uint8_t> itemNameBytes;
    rom.getDataChunk(0x29732, 0x29A0A, itemNameBytes);
    std::vector<std::vector<uint8_t>> itemNames;

    // "Kazalt Jewel" mode is a specific mode when user asked for more jewels than we can provide individual items for.
    // In that case, we only use one generic jewel item type which can be obtained several times, and check against this
    // item's count instead of checking if every jewel type is owned at Kazalt teleporter
    bool kazaltJewelMode = (options.getJewelCount() > MAX_INDIVIDUAL_JEWELS);

    // Read item names
    uint32_t addr = 0;
    while(true)
    {
        uint16_t stringSize = itemNameBytes[addr++];
        if(stringSize == 0xFF)
            break;

        // Clear Island Map name to make room for other names
        if(itemNames.size() == ITEM_ISLAND_MAP)
            itemNames.push_back(std::vector<uint8_t>({ 0x00 }));
        // Rename all default equipments with "None"
        else if(itemNames.size() == ITEM_NO_SWORD || itemNames.size() == ITEM_NO_ARMOR || itemNames.size() == ITEM_NO_BOOTS)
            itemNames.push_back({ 0x18, 0x33, 0x32, 0x29 });
        // Rename No52 into Green Jewel
        else if(itemNames.size() == ITEM_GREEN_JEWEL && !kazaltJewelMode)
            itemNames.push_back({ 0x11, 0x36, 0x29, 0x29, 0x32, 0x6A, 0x14, 0x29, 0x3B, 0x29, 0x30 });
        // Rename Detox Book into Blue Jewel
        else if(itemNames.size() == ITEM_BLUE_JEWEL && !kazaltJewelMode)
            itemNames.push_back({ 0x0C, 0x30, 0x39, 0x29, 0x6A, 0x14, 0x29, 0x3B, 0x29, 0x30 });
        // Rename AntiCurse Book into Yellow Jewel
        else if(itemNames.size() == ITEM_YELLOW_JEWEL && !kazaltJewelMode)
            itemNames.push_back({ 0x23, 0x29, 0x30, 0x30, 0x33, 0x3B, 0x6A, 0x14, 0x29, 0x3B, 0x29, 0x30 });
        // Clear "Purple Jewel" name to make room for other names since it's unused in Kazalt Jewel mode
        else if(itemNames.size() == ITEM_PURPLE_JEWEL && kazaltJewelMode)
            itemNames.push_back(std::vector<uint8_t>({ 0x00 }));
        // Rename "Red Jewel" into the more generic "Kazalt Jewel" in Kazalt Jewel mode
        else if(itemNames.size() == ITEM_RED_JEWEL && kazaltJewelMode)
            itemNames.push_back(std::vector<uint8_t>({ 0x15, 0x25, 0x3E, 0x25, 0x30, 0x38, 0x6A, 0x14, 0x29, 0x3B, 0x29, 0x30 }));
        // No specific treatment, just add it back as-is
        else
            itemNames.push_back(std::vector<uint8_t>(itemNameBytes.begin() + addr, itemNameBytes.begin() + addr + stringSize));

        addr += stringSize;
    }

    constexpr uint16_t initialSize = 0x29A0A - 0x29732;

    itemNameBytes.clear();
    for(const std::vector<uint8_t>& itemName : itemNames)
    {
        itemNameBytes.push_back((uint8_t)itemName.size());
        itemNameBytes.insert(itemNameBytes.end(), itemName.begin(), itemName.end());
    }
    itemNameBytes.push_back(0xFF);

    if(itemNameBytes.size() > initialSize)
        throw new RandomizerException("Item names size is above initial game size");
    rom.setBytes(0x29732, itemNameBytes);
}

void handleAdditionalJewels(md::ROM& rom, const RandomizerOptions& options)
{
    if(options.getJewelCount() > MAX_INDIVIDUAL_JEWELS)
        return;    

    constexpr uint32_t itemSpritesTableBaseAddr = 0x121578;
    
    if(options.getJewelCount() >= 3)
    {
        // Add a sprite for green jewel and make the item use it
        std::vector<uint8_t> greenJewelSprite;
        loadGreenJewelSprite(greenJewelSprite);
        uint32_t greenJewelSpriteAddr = rom.injectBytes(greenJewelSprite);
        rom.setLong(itemSpritesTableBaseAddr + (ITEM_GREEN_JEWEL * 0x4), greenJewelSpriteAddr); // 0x121648
    }
    if(options.getJewelCount() >= 4)
    {
        // Add a sprite for blue jewel and make the item use it
        std::vector<uint8_t> blueJewelSprite;
        loadBlueJewelSprite(blueJewelSprite);
        uint32_t blueJewelSpriteAddr = rom.injectBytes(blueJewelSprite);
        rom.setLong(itemSpritesTableBaseAddr + (ITEM_BLUE_JEWEL * 0x4), blueJewelSpriteAddr);
    }
    if(options.getJewelCount() >= 5)
    {
        // Add a sprite for green jewel and make the item use it
        std::vector<uint8_t> yellowJewelSprite;
        loadYellowJewelSprite(yellowJewelSprite);
        uint32_t yellowJewelSpriteAddr = rom.injectBytes(yellowJewelSprite);
        rom.setLong(itemSpritesTableBaseAddr + (ITEM_YELLOW_JEWEL * 0x4), yellowJewelSpriteAddr);
    }

    // Remove jewels replaced from book IDs from priest stands
    constexpr uint8_t emptyItem = ITEM_NONE + 0xC0;
    rom.setByte(0x21037, emptyItem); // Massan Blue Jewel
    rom.setByte(0x2103F, emptyItem); // Massan Yellow Jewel
    rom.setByte(0x21193, emptyItem); // Gumi Blue Jewel
    rom.setByte(0x2119B, emptyItem); // Gumi Yellow Jewel
    rom.setByte(0x21287, emptyItem); // Ryuma Blue Jewel
    rom.setByte(0x2128F, emptyItem); // Ryuma Yellow Jewel
    rom.setByte(0x21D77, emptyItem); // Mercator Blue Jewel
    rom.setByte(0x21D6F, emptyItem); // Mercator Yellow Jewel
    rom.setByte(0x21F79, emptyItem); // Verla Blue Jewel
    rom.setByte(0x21F71, emptyItem); // Verla Yellow Jewel
    rom.setByte(0x22099, emptyItem); // Destel Blue Jewel
    rom.setByte(0x220A1, emptyItem); // Destel Yellow Jewel
    rom.setByte(0x22137, emptyItem); // Kazalt Blue Jewel
    rom.setByte(0x2213F, emptyItem); // Kazalt Yellow Jewel

    // Make the Awakening Book (the only one remaining in churches) heal all status conditions
    rom.setCode(0x24F6C, md::Code().nop(6));
    rom.setCode(0x24FB8, md::Code().moveb(0xFF, reg_D0));
}

void makePawnTicketConsumable(md::ROM& rom)
{
    md::Code procConsumePawnTicket;
    procConsumePawnTicket.jsr(0x8B98); // ConsumeItem
    procConsumePawnTicket.nop(3);
    rom.setCode(0x88D2, procConsumePawnTicket);
}

void makeKeyNotConsumedOnUse(md::ROM& rom)
{
    rom.setCode(0x8B34, md::Code().nop());
}

void alterCredits(md::ROM& rom)
{
    constexpr uint32_t creditsTextBaseAddr = 0x9ED1A;

    // Change "LANDSTALKER" to "RANDSTALKER"
    rom.setByte(creditsTextBaseAddr + 0x2, 0x13);

    rom.setBytes(creditsTextBaseAddr + 0x14, { 
        0x27, 0x1C, 0x29, 0x1F, 0x2E, 0x2F, 0x1C, 0x27, 0x26, 0x20, 0x2D, 0x80, // "landstalker "
        0x2D, 0x1C, 0x29, 0x1F, 0x2A, 0x28, 0x24, 0x35, 0x20, 0x2D              // "randomizer"
    });

    // Widen the space between the end of the cast and the beginning of the rando staff
    rom.setByte(creditsTextBaseAddr + 0x5C, 0x0F);

    rom.setBytes(creditsTextBaseAddr + 0x75, { 
        // RANDOMIZER
        0x08, 0xFF, 0x82, 
        0x80, 0x13, 0x80, 0x02, 0x80, 0x0F, 0x80, 0x05, 0x80, 0x10, 0x80, 0x0E, 0x80, 0x0A, 0x80, 0x1B, 0x80, 0x06, 0x80, 0x13, 0x80, 0x00, 
        // developed by
        0x04, 0xFF, 0x81, 
        0x1F, 0x20, 0x31, 0x20, 0x27, 0x2A, 0x2B, 0x20, 0x1F, 0x80, 0x1D, 0x34, 0x00,
        // DINOPONY
        0x03, 0xFF,
        0x05, 0x0A, 0x0F, 0x10, 0x11, 0x10, 0x0F, 0x1A, 0x00,
        // WIZ
        0x08, 0xFF,
        0x18, 0x0A, 0x1B, 0x00,
        // testing
        0x04, 0xFF, 0x81,
        0x2F, 0x20, 0x2E, 0x2F, 0x24, 0x29, 0x22, 0x00,
        // SAGAZ
        0x03, 0xFF,
        0x14, 0x02, 0x08, 0x02, 0x1B, 0x00,
        // STIK
        0x03, 0xFF,
        0x14, 0x15, 0x0A, 0x0C, 0x00,
        // WIZ
        0x03, 0xFF,
        0x18, 0x0A, 0x1B, 0x00,
        // LANDRYLE
        0x08, 0xFF,
        0x0D, 0x02, 0x0F, 0x05, 0x13, 0x1A, 0x0D, 0x06, 0x00,
        // website by
        0x04, 0xFF, 0x81,
        0x32, 0x20, 0x1D, 0x2E, 0x24, 0x2F, 0x20, 0x80, 0x1D, 0x34, 0x00,
        // DILANDAU
        0x08, 0xFF,
        0x05, 0x0A, 0x0D, 0x02, 0x0F, 0x05, 0x02, 0x16, 0x00
    });

    std::vector<uint8_t> rest;
    rom.getDataChunk(creditsTextBaseAddr + 0x1D2, creditsTextBaseAddr + 0x929, rest);
    rom.setBytes(creditsTextBaseAddr + 0xF5, rest);
    for(uint32_t addr = creditsTextBaseAddr + 0xF5 + (uint32_t)rest.size() ; addr <= creditsTextBaseAddr + 0x929 ; ++addr)
        rom.setByte(addr, 0x00);
}

void applyPatches(md::ROM& rom, const RandomizerOptions& options, const World& world)
{    
    // Game & gameplay changes
    alterGameStart(rom, options, world);
    alterItemOrderInMenu(rom);
    alterLifestockHandlingInShops(rom);
    addStatueOfJyptaGoldsOverTime(rom);
    quickenGaiaEffect(rom);
    addRecordBookSave(rom, options);
    alterFahlChallenge(rom);
    addJewelsCheckForTeleporterToKazalt(rom, options);
    addFunctionToItemsOnUse(rom);
    alterGoldRewardsHandling(rom);
    alterLanternHandling(rom);
    handleAdditionalJewels(rom, options);
    makePawnTicketConsumable(rom);
    makeKeyNotConsumedOnUse(rom);
    if (options.useArmorUpgrades())
        handleArmorUpgrades(rom);

    // Flag-reading changes (from story flag reading to inventory reading)
    fixAxeMagicCheck(rom);
    fixSafetyPassCheck(rom);
    fixArmletCheck(rom);
    fixSunstoneCheck(rom);
    fixDogTalkingCheck(rom);
    alterMercatorSecondaryShopCheck(rom);
    alterCasinoTicketHandling(rom);
    
    // Specific map check changes
    alterWaterfallShrineSecretStairsCheck(rom);
    makeFallingRibbonAlwaysThere(rom);
    alterKingNolesCaveTeleporterCheck(rom);
    makeMercatorDocksShopAlwaysOpen(rom);
    makeArthurAlwaysPresentInThroneRoom(rom);
    fixCryptBehavior(rom);
    fixKingNolesLabyrinthRafts(rom);
    removeLogsRoomExitCheck(rom);
    fixMirFightAfterLakeShrine(rom);
    fixMirTowerPriestRoomItems(rom);
    makeSwordOfGaiaWorkInVolcano(rom);
    fixReverseGreenmazeFountainSoftlock(rom);
    addDoorForReverseSafetyPass(rom);
    makeRyumaMayorSaveable(rom);

    // Specific map content changes
    removeMercatorCastleBackdoorGuard(rom);
    removeSailorInDarkPort(rom);
    fixFaraLifestockChest(rom);
    replaceLumberjackByChest(rom);
    replaceSickMerchantByChest(rom);
    replaceFaraInElderHouseByChest(rom);

    // Fix original game glitches & bugs
    if(options.fixArmletSkip())
        fixArmletSkip(rom);
    if(options.fixTreeCuttingGlitch())
        fixTreeCuttingGlitch(rom);

    // Miscellaneous
    deactivateRegionCheck(rom);
    changeHUDColor(rom, options);
    shortenMirCutsceneAfterLakeShrine(rom);
    renameItems(rom, options);
    alterCredits(rom);
}
