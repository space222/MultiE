#include <thread>
#include <print>
#include <iostream>
#include <sockpp/tcp_acceptor.h>
#include <atomic>
#include "console.h"
extern console* sys;
std::atomic<int> gdb_active = 0;
extern bool Paused;

static std::vector<u32> breakpoints;

u32 str2int(const std::string_view& s, u8 v)
{
	u32 res = 0;
	u32 i = 0;
	while( i < s.size() && s[i] != v )
	{
		res <<= 4;
		char c = s[i];
		if( c >= 'a' && c <= 'f' ) { res |= c-'a'+10; }
		else { res |= c-'0'; }
		i+=1;
	}
	return res;
}

void append_checksum(std::string& s)
{
	u8 ch = 0;
	for(u32 i = 2; i < s.size(); ++i)
	{
		ch += s[i];
	}
	s += '#';
	s += (ch>>4) + ((ch>>4)>9 ? ('a'-10) : '0');
	s += (ch&0xf) + ((ch&0xf)>9 ? ('a'-10) : '0');
}

void gdb_exec(sockpp::tcp_socket& sock, std::string msg)
{
	std::string_view cmd(msg.begin() + msg.find('$') + 1, msg.begin() + msg.find_first_of(":#"));
	std::println("cmd: <{}>", cmd);
	
	if( cmd[0] == 'H' )
	{
		std::string res = "+$";
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;
	}
	if( cmd[0] == 'm' )
	{
		std::string res = "+$000006EF";
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;		
	}
	if( cmd == "p19" )
	{
		std::string res = "+$00000000";
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;		
	}
	if( cmd.starts_with('p') )
	{
		std::string res = (cmd[1]=='f' ? "+$00000008":"+$00000000");
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;		
	}
	if( cmd == "qTStatus" )
	{
		std::string res = "+$";
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;
	}
	if( cmd == "qSupported" )
	{
		std::println("gdb support is <{}>", msg);
		std::string res = "+$vContSupported+"; //binary-upload+;
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;
	}
	if( cmd == "vCont?" )
	{
		std::string res = "+$";
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;
	}
	if( cmd == "vMustReplyEmpty" || cmd == "qfThreadInfo" 
	 || cmd.starts_with("qL") || cmd == "qC" 
	 || cmd == "qSymbol" )
	{
		std::string res = "+$";
		append_checksum(res);
		//std::println("empty reply = <{}>", res);
		sock.write_n(res.data(), res.size());
		return;
	}
	if( cmd == "qAttached" )
	{
		std::string res = "+$";
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;
	}
	if( cmd == "?" )
	{
		std::string res = "+$S05";
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;
	}
	if( cmd == "g" )
	{
		std::string res = "+$00000000";
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;
	}
	if( cmd == "qOffsets" )
	{
		std::string res = "+$Text=0;Data=0;Bss=0";
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;
	}
	if( cmd[0] == 'Z' )
	{
		u32 bp = str2int(std::string_view(msg.begin() + msg.find(',') + 1, msg.end()), ',');
		std::println("adding breakpoint at ${:X}", bp);
		std::string res = "+$OK";
		append_checksum(res);
		sock.write_n(res.data(), res.size());
		return;
	}
	std::println();
}

void gdb_stub(sockpp::tcp_socket sock)
{
	std::string msg;
	sockpp::result<size_t> res;

	while(true)
	{
		char c;
		while( (res = sock.read(&c, 1)) && res.value() == 1 )
		{
			if( c == 3 )
			{
				Paused = true;
				std::println("Did a break");
			}
			if( msg.empty() && (c == '+' || c == '-') ) continue;
			msg += c;
			if( msg.starts_with("$") && msg.size() > 3 && msg[msg.size()-3] == '#' )
			{
				gdb_exec(sock, msg);
				msg.clear();
			}
		}
		if( res.value() < 1 ) break;
	}

	std::cout << "Connection closed from " << sock.peer_address() << std::endl;
}

void gdb_start()
{
	gdb_active.store(1);
	
	auto go = []() 
	{ 
		try {
			int16_t port = 9912;
			std::error_code ec;
			sockpp::tcp_acceptor acc(port, sockpp::tcp_acceptor::DFLT_QUE_SIZE, sockpp::tcp_acceptor::REUSE, ec);
			auto res = acc.accept();
			gdb_stub(res.release());
		} catch(std::system_error &e) {
			std::println("Unable to start GDB stub: {}", e.what());
		}
		gdb_active.store(0);
	};
	
	std::jthread t(go);
	t.detach();
}

