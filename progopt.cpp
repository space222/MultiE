#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string>
#include "progopt.h"

/*
Very simple, unconfigurable, program options handling.

Supports both one and two dashes, maybe, if nothing weird gets passed.
They are treated the same, by stripping the dashes and using the next
thing as the arg to that option. Only one arg per option. Constructor
takes optional list of flag-only option names. Any parsing of results 
(eg into int) is responsibility of the user.

If an option has an equal sign (=) DIRECTLY AFTER IT WITH NO SPACES,
that will be handled, even if an errant space separates the "-option="
from the arg.

"-option = arg" will result in "-option" having the arg "=", with "arg"
being in the 'files' array. Don't do that.

If you need double dashes to do something else, the spot to tweak is labeled below.
*/

ProgramOptions::ProgramOptions(int argc, char** args, std::vector<std::string> flag_only)
{
	if( argc < 2 ) return;
	size_t num_args = (size_t) argc;
	bool put_it = false;
	std::string_view opt;
	for(size_t A = 1; A < num_args; ++A)
	{
		std::string_view t = args[A];
		if( t[0] == '-' )
		{
			if( put_it )
			{
				options.insert(std::pair(std::string(opt), std::string()));
			}
			if( t.size() < 2 ) continue;
			if( t[1] == '-' )  // <-- where you'd need to start to mod double-dash
			{
				opt = t.substr(2);
			} else {
				opt = t.substr(1);
			}
			if( opt.find_first_not_of(" \t") == std::string_view::npos )
			{
				continue; // not currently supporting bare - or --
			}
			auto pos = opt.find('=');
			if( pos != std::string::npos )
			{
				std::string_view name = opt.substr(0,pos);
				std::string_view value = opt.substr(pos+1);
				if( value.find_last_not_of(" \t") == std::string_view::npos )
				{
					opt = name;
					put_it = true;
				} else {
					options.insert(std::pair(std::string(name), std::string(value)));
				}
			} else {
				if( std::find(std::begin(flag_only), std::end(flag_only), opt) == std::end(flag_only) )
				{
					put_it = true;
				} else {
					options.insert(std::pair(std::string(opt), std::string()));
				}
			}
		} else if( put_it ) {
			put_it = false;
			options.insert(std::pair(std::string(opt), std::string(t)));
		} else {
			files.push_back(std::string(t));
		}
	}
	
	// handles argless option at end of command line
	if( put_it )
	{
		options.insert(std::pair(std::string(opt), std::string()));
	}
	return;
}


