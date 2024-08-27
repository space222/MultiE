#pragma once
#include <vector>
#include <unordered_map>
#include <string>

class ProgramOptions
{
public:
	ProgramOptions(int argc, char** args, std::vector<std::string> flag_only = {});

	std::unordered_map<std::string, std::string> options;
	std::vector<std::string> files;
};


