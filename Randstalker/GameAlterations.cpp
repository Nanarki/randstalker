#include "GameAlterations.h"
#include <cstdint>

constexpr auto INIT_FUNCTION_INJECTION_ADDRESS = 0x1FFAD0;

constexpr auto OPCODE_MOVB =	0x13FC;
constexpr auto OPCODE_MOVW =	0x33FC;
constexpr auto OPCODE_RTS =		0x4E75;
constexpr auto OPCODE_JSR =		0x4EB9;
constexpr auto OPCODE_NOP =		0x4E71;
constexpr auto OPCODE_BRA =		0x6000;
constexpr auto OPCODE_BNE =		0x6600;
constexpr auto OPCODE_BEQ =		0x6700;

void alterGameStart(GameROM& rom)
{
	// ------- Change default map --------
	// Change the starting map from the intro cutscene map to Massan Inn

	// 0x0027F2:
		// Before:	[33FC] move.w 0x008B -> $FF1204
		// After:	[33FC] move.w 0x0258 -> $FF1204
	rom.setWord(0x0027F4, 0x0258);

	// ------- PosX & PosZ ---------
	// Change the initial position of Nigel so that it fits to the new spawn map

	// 0x0027FA: 
		// Before: 	[13FC] move.b 0x3E -> $FF5400
		// After:	[13FC] move.b 0x1F -> $FF5400
	rom.setByte(0x0027FD, 0x1F);

	// 0x002802: 
		// Before: 	[13FC] move.b 0x18 -> $FF5401
		// After:	[13FC] move.b 0x19 -> $FF5401
	rom.setByte(0x002805, 0x19);

	// ------- Remove no music flag ---------
	// Replace the bitset of the no music flag by a jump to the injected Randstalker init function located at the end of the rom

	// 0x002700:
		// Before: 	[08F9] bset 3 -> $FF1027
		// After:	[4EB9] jsr $1FFAD0	; 	[4E71] nop
	rom.setWord(0x002700, OPCODE_JSR);
	rom.setLong(0x002702, INIT_FUNCTION_INJECTION_ADDRESS);
	rom.setWord(0x002706, OPCODE_NOP); // transform last two bytes into a NOP to keep the ROM padding intact
	
	// ------- Remove cutscene flag (no input allowed) ---------
	// Usually, when starting a new game, it is automatically put into "cutscene mode" to let the intro roll without allowing the player
	// to move or pause, or do anything at all. We need to remove that cutscene flag to enable the player actually playing the game.

	// 0x00281A:
		// Before:	[33FC] move.w 0x00FE -> $FF12DE
		// After:	[4E71] nop (4 times)
	rom.setWord(0x00281A, OPCODE_NOP); // NOP
	rom.setWord(0x00281C, OPCODE_NOP); // NOP
	rom.setWord(0x00281E, OPCODE_NOP); // NOP
	rom.setWord(0x002820, OPCODE_NOP); // NOP


	// ------- Inject init function ---------
	// Init function is used to set story flags to specific values at the very beginning of the game, opening some usually closed paths
	// and removing some useless cutscenes (considering them as "already seen").

	uint32_t address = INIT_FUNCTION_INJECTION_ADDRESS;

	// move.b 0x00B1 -> $FF1000
	rom.setWord(address, OPCODE_MOVB);	address += 0x02;
	rom.setWord(address, 0x00B1);		address += 0x02;
	rom.setLong(address, 0x00FF1000);	address += 0x04;

	// move.w 0xD5E0 -> $FF1002
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0xD5E0);		address += 0x02;
	rom.setLong(address, 0x00FF1002);	address += 0x04;

	// move.w 0xDF60 -> $FF1004
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0xDF60);		address += 0x02;
	rom.setLong(address, 0x00FF1004);	address += 0x04;

	// move.w 0x7EB4 -> $FF1006
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0x7EB4);		address += 0x02;	
	rom.setLong(address, 0x00FF1006);	address += 0x04;

	// move.w 0xFE7E -> $FF1008
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0xFE7E);		address += 0x02;
	rom.setLong(address, 0x00FF1008);	address += 0x04;

	// move.w 0x1E03 -> $FF1012
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0x1E03);		address += 0x02;
	rom.setLong(address, 0x00FF1012);	address += 0x04;

	// move.w 0x8100 -> $FF1014
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0x8100);		address += 0x02;
	rom.setLong(address, 0x00FF1014);	address += 0x04;

	// move.w 0x0048 -> $FF1016
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0x0048);		address += 0x02;
	rom.setLong(address, 0x00FF1016);	address += 0x04;

	// move.w 0x7000 -> $FF1020
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0x7000);		address += 0x02;
	rom.setLong(address, 0x00FF1020);	address += 0x04;

	// move.w 0x8020 -> $FF1026
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0x8020);		address += 0x02;
	rom.setLong(address, 0x00FF1026);	address += 0x04;

	// move.w 0xC040 -> $FF1028
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0xC040);		address += 0x02;
	rom.setLong(address, 0x00FF1028);	address += 0x04;

	// move.w 0x8180 -> $FF102A
	rom.setWord(address, OPCODE_MOVW);	address += 0x02;
	rom.setWord(address, 0x8180);		address += 0x02;
	rom.setLong(address, 0x00FF102A);	address += 0x04;

	// rts (return from function)
	rom.setWord(address, OPCODE_RTS);
}

void fixAxeMagicCheck(GameROM& rom)
{
	// Changes the Axe Magic check when slashing a tree from bit 0 of flag 1003 to "Axe Magic owned"
	
	// 0x16262:
		// Before:	[0839] btst 0 in FF1003
		// After:	[0839] btst 5 in FF104B
	rom.setWord(0x016264, 0x0005);
	rom.setWord(0x016268, 0x104B);
}

void fixSafetyPassCheck(GameROM& rom)
{
	// Change Mercator door opened check from bit 7 of flag 1004 to "Safety Pass owned"

	// 0x004FF8:
		// Before:	04 0F (0F & 7 = 7 -----> bit 7 of FF1004)
		// After:	59 0D (0D & 7 = 5 -----> bit 5 of FF1059)
	rom.setWord(0x004FF8, 0x590D);
}

void fixArmletCheck(GameROM& rom)
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
	rom.setWord(0x013A8A, OPCODE_RTS);
}

void fixSunstoneCheck(GameROM& rom)
{
	// Change Sunstone check for repairing the lighthouse from bit 2 of flag 1026 to "Susntone owned"
	// 0x09D091:
		// Before:	26 02 (bit 2 of FF1026)
		// After:	4F 01 (bit 1 of FF104F)
	rom.setWord(0x09D091, 0x4F01);
}

void fixDogTalkingCheck(GameROM& rom)
{
	// Change doggo talking check from bit 4 of flag 1024 to "Einstein Whistle owned"
	
	// 0x0253C0:
		// Before:	01 24 (0124 >> 3 = 24 and 0124 & 7 = 04 -----> bit 4 of FF1024)
		// After:	02 81 (0281 >> 3 = 50 and 0281 & 7 = 01 -----> bit 1 of FF1050)
	rom.setWord(0x0253C0, 0x0281);
}

void fixCryptBehavior(GameROM& rom)
{
	// 1) Remove the check "if shadow mummy was beaten, raft mummy never appears again"
	
	// 0x019DF6:
		// Before:	0839 0006 00FF1014 (btst bit 6 in FF1014) ; 66 14 (bne $19E14)
		// After:	4EB9 00019E14 (jsr $19E14; 4E71 4E71 (nop nop)
	rom.setWord(0x019DF6, OPCODE_NOP);
	rom.setWord(0x019DF8, OPCODE_NOP);
	rom.setLong(0x019DFA, OPCODE_NOP);
	rom.setWord(0x019DFC, OPCODE_NOP);
	rom.setWord(0x019DFE, OPCODE_NOP);

	// 2) Change the room exit check and shadow mummy appearance from "if armlet is owned" to "chest was opened"

	// 0x0117E8:
		// Before:	103C 001F ; 4EB9 00022ED0 ; 4A41 ; 6B00 F75C (bmi $10F52)
		// After:	0839 0002 00FF1097 (btst 2 FF1097)	; 6700 F75C (bne $10F52)
	rom.setWord(0x0117E8, 0x0839);		// btst
	rom.setWord(0x0117EA, 0x0002);		// bit 2
	rom.setLong(0x0117EC, 0x00FF1097);	// of flag FF1097
	rom.setWord(0x0117F0, OPCODE_NOP);	// nop
	rom.setWord(0x0117F2, OPCODE_NOP);	// nop
	rom.setWord(0x0117F4, OPCODE_BEQ);	// bne $10F52
}

void fixMirAfterLakeShrineCheck(GameROM& rom)
{
	// 0x01AA22:
		// Before:	0310 2A A2 (in map 310, check bit 5 of flag 102A)
		// After:	0000 00 00 (game, please do nothing. simple.)
	rom.setWord(0x01AA22, 0x0000);
	rom.setWord(0x01AA24, 0x0000);
}

void fixLogsRoomExitCheck(GameROM& rom)
{
	// Remove logs check
	rom.setWord(0x011EC4, OPCODE_BRA);
}

void alterCasinoCheck(GameROM& rom)
{
	// Change the Casino entrance check so that the NPC is always out of the way

	// 0x09DF25:
		// Before:	29 03 (bit 3 of FF1029)
		// After:	00 00 (bit 0 of FF1000 - always true) 
	rom.setWord(0x09DF25, 0x4B02);
}

void alterMercatorSecondaryShopCheck(GameROM& rom)
{
	// Change the Mercator secondary shop check so that it sells item as long as you own Buyer's Card

	// 0x00A574:
		// Before:	2A 04 (bit 4 of FF102A)
		// After:	4C 05 (bit 5 of FF104C)
	rom.setWord(0x00A574, 0x4C05);
}

void alterWaterfallShrineSecretStairsCheck(GameROM& rom)
{
	// Change Waterfall Shrine entrance check from "Talked to Prospero" to "What a noisy boy!", removing the need
	// of talking to Prospero (which we couldn't do anyway because of the story flags).

	// 0x005014:
		// Before:	00 08 (bit 0 of FF1000)
		// After:	02 09 (bit 1 of FF1002)
	rom.setWord(0x005014, 0x0209);
}

void alterROM(GameROM& rom)
{
	alterGameStart(rom);
	
	fixAxeMagicCheck(rom);
	fixSafetyPassCheck(rom);
	fixArmletCheck(rom);
	fixSunstoneCheck(rom);
	fixDogTalkingCheck(rom);
	fixCryptBehavior(rom);
	fixMirAfterLakeShrineCheck(rom);
	fixLogsRoomExitCheck(rom);
	alterCasinoCheck(rom);
	alterMercatorSecondaryShopCheck(rom);
	alterWaterfallShrineSecretStairsCheck(rom);
}