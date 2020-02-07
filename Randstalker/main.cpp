//////////////////////////////////////////////////////////////////////////////////////////
//
//		RANDSTALKER ROM GENERATOR
//
// ---------------------------------------------------------------------------------------
//
//		Developed by:	Dinopony (@DinoponyRuns)
//		Version:		v0.9e
//
// ---------------------------------------------------------------------------------------
//
//	Special thanks to the whole Landstalker speedrunning community for being supportive
//	and helpful during the whole process of developing this!
//
//
//  Command line syntax:
//		randstalker [args]
//
//	Common parameters:
//		--inputRom="value"		===> Path to the game ROM used as input for the randomization (this file will only be read, not modified).
//		--outputRom="value"		===> Path where the randomized ROM will be put, defaults to 'output.md' in current working directory.
//		--seed="value"			===> Random seed (integer value or "random") used to alter the game. Using the same seed twice will produce the same result.
//		--outputLog="value"		===> Path where the seed log will be put, defaults to 'randstalker.log' in current working directory.
//		--noPause				===> Don't ask to press a key at the end of generation (useful for automated generation systems)
//
//	Randomization options:
//		--shuffleTrees			===> Randomize Tibor trees
//		--noArmorUpgrades		===> Don't use armor upgrades, just place vanilla armors randomly
//		--randomSpawn			===> randomize spawn point between Massan, Gumi and Ryuma
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <string>
#include <iostream>
#include <map>
#include <chrono>

#include "GameROM.h"
#include "GameAlterations.h"
#include "Tools.h"
#include "World.h"
#include "WorldRandomizer.h"

constexpr auto RELEASE = "0.9e";

std::map<std::string, std::string> parseOptions(int argc, char* argv[])
{
	std::map<std::string, std::string> options;

	for (int i = 1; i < argc; ++i)
	{
		std::string param = argv[i];
//		Tools::trim(param);
		if (param[0] != '-' || param[1] != '-')
			continue;

		auto tokenIter = std::find(param.begin() + 2, param.end(), '=');
		std::string paramName(param.begin() + 2, tokenIter);
		Tools::toLower(paramName);
		if (tokenIter != param.end())
			options[paramName] = std::string(tokenIter + 1, param.end());
		else
			options[paramName] = "true";
	}

	return options;
}

uint32_t getSeedFromOptionsOrPrompt(const std::map<std::string, std::string>& options)
{
	bool randomSeed = false;

	std::string inputtedSeedString;
	try {
		inputtedSeedString = options.at("seed");
	}
	catch(std::out_of_range&) {
		std::cout << "Please specify the seed (blank for random seed): ";
		std::getline(std::cin, inputtedSeedString);
	}

	if (inputtedSeedString != "random" && !inputtedSeedString.empty())
	{
		try {
			return (uint32_t)std::stoul(inputtedSeedString);
		}
		catch (std::invalid_argument&) {
			std::cout << "Provided seed '" << inputtedSeedString << "' is invalid, a random seed will be used instead.\n";
		}
	}

	return (uint32_t)std::chrono::system_clock::now().time_since_epoch().count();
}

GameROM* getInputROM(std::map<std::string, std::string>& options, int argc, char* argv[])
{
	std::string inputRomPath = options["inputrom"];
	if (inputRomPath.empty() && argc >= 2)
	{
		std::string param = argv[1];
		if (param.size() > 2 && param[0] != '-' && param[1] != '-')
			inputRomPath = param;
	}

	GameROM* rom = new GameROM(inputRomPath);
	while (!rom->isValid())
	{
		delete rom;
		if (!inputRomPath.empty())
			std::cout << "[ERROR] ROM input path \"" << inputRomPath << "\" is wrong, and no ROM could be opened this way.\n\n";
		std::cout << "Please specify input ROM path (or drag ROM on Randstalker.exe icon before launching): ";
		std::getline(std::cin, inputRomPath);
		rom = new GameROM(inputRomPath);
	}

	return rom;
}

int main(int argc, char* argv[])
{
	std::cout << "======== Randstalker v" << RELEASE << " ========\n\n";

	std::map<std::string, std::string> options = parseOptions(argc, argv);
	for (auto it = options.begin(); it != options.end(); ++it)
		std::cout << "NAME = `" << it->first << "`, VALUE = `" << it->second << "`\n";

	// ------------- Input ROM path -------------
	GameROM* rom = getInputROM(options, argc, argv);
	std::cout << "\n";

	// ------------ Seed -------------
	uint32_t seed = getSeedFromOptionsOrPrompt(options);
	std::cout << "Used seed: " << seed << "\n\n";

	// ------------ Output ROM path -------------
	std::string outputRomPath = options["outputrom"];
	if (outputRomPath.empty())
		outputRomPath = "output.md";

	// ------------ Output log path -------------
	std::string outputLogPath = options["outputlog"];
	if (outputLogPath.empty())
		outputLogPath = "randstalker.log";

	// ------------ Randomization -------------
	// Perform game changes unrelated with the randomization part
	alterROM(*rom, options);
	
	// Create a replica model of Landstalker world, randomize it and save it to the ROM	
	World landstalkerWorld(options);

	std::ofstream logFile(outputLogPath);
	WorldRandomizer randomizer(landstalkerWorld, seed, logFile, options);
	randomizer.randomize();
	logFile.close();

	landstalkerWorld.writeToROM(*rom);
	rom->saveAs(outputRomPath);

	std::cout << "Randomized rom outputted to \"" << outputRomPath << "\".\n\n";

	if ( ! options.count("nopause") )
	{
		std::cout << "Press any key to exit.";
		std::string dummy;
		std::getline(std::cin, dummy);
	}

	return 0;
}
