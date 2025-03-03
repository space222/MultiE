#pragma once
#include <string>
#include <vector>
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
		} else if constexpr( std::same_as<T, std::vector<std::string>> ) {
			std::vector<std::string> res;
			auto* arr = tbl[sys][prop].as_array();
			if( arr )
			{
				for(auto& el : *arr)
				{
					res.push_back(el.value_or<std::string>(""));
				}
			}
			return res;
		}
		return {};
	}
	
	static bool mute;
	
protected:
	static toml::table tbl;	
};


