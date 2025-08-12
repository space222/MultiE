#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <filesystem>
#include "itypes.h"
#define TOML_IMPLEMENTATION
#include "Settings.h"

static const char* conf_file_name = "config.toml";

Settings::Settings()
{
	if( ! std::filesystem::exists(conf_file_name) )
	{
		FILE* fp = fopen(conf_file_name, "wb");
		if( ! fp )
		{
			printf("Unable to create config file\n");
			exit(1);
		}
		//todo: output some default config
	}

	try
	{
		tbl = toml::parse_file(conf_file_name);
		//std::cout << "Config:\n" << tbl << "\n";
	} catch (const toml::parse_error& err) {
		std::cerr << "Settings: Parsing failed:\n" << err << "\n";
		return;
	}
}

bool Settings::mute = false;
toml::table Settings::tbl;
static Settings conf;










