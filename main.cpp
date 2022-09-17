/*
9/17/2022
lua thread API
*/

#include <lua.hpp>
#include <stdio.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

int l_sleep(lua_State* l)
{
	std::this_thread::sleep_for(std::chrono::milliseconds((int) luaL_checkinteger(l, 1)));
	return 1;
}

int l_log(lua_State* l)
{
	printf("%s\n", luaL_checkstring(l, 1));
	return 1;
}

int l_rstring(lua_State* l)
{
	int len = luaL_checkinteger(l, 1);
	const char* base = "aA";
	std::string ret = "";
	for (int i = 0; i < len; i++) ret += (char)(base[rand() % 2] + rand() % 26);
	lua_pushstring(l, ret.c_str());
	return 1;
}

// storing ids of thread
std::vector<std::string> _lua_threads;
int l_t_pass(lua_State* l)
{
	_lua_threads.push_back(std::string(luaL_checkstring(l, 1)));
	return 1;
}

void l_exec(lua_State* l, std::string content)
{
	l = lua_newthread(l);
	if (luaL_dostring(l, content.c_str())) std::cout << luaL_checkstring(l, -1) << std::endl;
}

void l_exec_thread(lua_State* l, std::string id)
{
	l = lua_newthread(l);
	if (lua_getglobal(l, "thread_run") == LUA_TFUNCTION) {
		lua_pushstring(l, id.c_str());
		if (lua_pcall(l, 1, 0, 0)) std::cout << luaL_checkstring(l, -1) << std::endl;
	}
}

int main()
{
	lua_State* l = luaL_newstate();
	
	// init
	luaL_openlibs(l);
	lua_register(l, "sleep", l_sleep);
	lua_register(l, "log", l_log);
	lua_register(l, "randomString", l_rstring);
	
	// thread functions
	lua_register(l, "thread_pass", l_t_pass);
	if (luaL_dostring(l,
	"local threads = {}\n"
	
	"function thread(func)\n"
	"if type(func) == 'function' then\n"
	"local id = randomString(math.random(5, 8))\n"
	"threads[id] = func\n"
	"thread_pass(id)\n"
	"end\n"
	"end\n"
	
	"function thread_run(id)\n"
	"if type(threads[id]) == 'function' then\n"
	"pcall(threads[id])\n"
	"threads[id] = nil\n"
	"end\n"
	"end")) std::cout << luaL_checkstring(l, -1) << std::endl;
	
	// run script
	std::thread throne(l_exec, l,
	"log('script start')\n"
	
	"function thread1()\n"
	"while true do\n"
	"log('yoo! from thread1')\n"
	"sleep(1000)\n"
	"end\n"
	"end\n"
	"thread(thread1)\n" // create thread for thread1
	
	"function thread2()\n"
	"while true do\n"
	"log('hello! from thread2')\n"
	"sleep(500)\n"
	"end\n"
	"end\n"
	"thread(thread2)\n" // create thread for thread2
	"thread(thread2)\n" // again?
	
	"thread(function()\n" // create thread without defining function
	"while true do\n"
	"log('i dont have function name')\n"
	"sleep(700)\n"
	"end\n"
	"end)\n"
	
	"sleep(1000)\n"
	"log('end of script')");
	throne.detach();
	
	bool ready = false; // dont thread at first loop
	while (true) {
		if (ready && _lua_threads.size() > 0) {
			
			// run thread
			std::thread oomaga(l_exec_thread, l, _lua_threads[0]);
			oomaga.detach();
			_lua_threads.erase(_lua_threads.begin());
			
		} else {
			ready = true;
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}
	
	return 1;
}
