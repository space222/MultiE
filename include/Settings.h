#pragma once
#include <string>
#include <optional>
#include "itypes.h"
#define TOML_HEADER_ONLY 0
#include "toml.hpp"

class Settings
{
public:
	Settings();
	
	template <typename T>
	static T get(std::string sys, std::string prop)
	{
		if constexpr( std::same_as<T, std::string> )
		{
			return tbl[sys][prop].value_or("");
		} else if constexpr( std::same_as<T, u32> ) {
			std::string s = tbl[sys][prop].value_or("");
			return std::atoi(s.c_str());
		} else if constexpr( std::same_as<T, std::vector<u32>> ) {
		
			return {};
		}
		return "";
	}

	static bool mute;
	
protected:
	static toml::table tbl;	
};


