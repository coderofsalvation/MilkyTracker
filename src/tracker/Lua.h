/*
 *  tracker/Lua.h
 *
 *  Copyright 2022 coderofsalvation/Leon van Kammen
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MILKYLUA_H
#define __MILKYLUA_H

#define LUA_IMPL
#include "../lua/minilua.h"
#include "../lua/compiled.h"

#define VERSION 0.1
        
#ifdef __cplusplus
class Lua {

    public:

#endif

#ifdef __cplusplus
    static inline pp_int32 getTableInt( lua_State *L, const char *table, const char *key){
#else 
    int getTableInt( lua_State *L, const char *table, const char *key){
#endif
    pp_int32 v = 0;
    lua_getglobal(L, table);
    if (lua_istable(L, -1))  {
        lua_getfield(L, -1, key);
        v = lua_tonumber(L, -1);
    }
    return v;
}

#ifdef __cplusplus
    static inline void initDSP(lua_State *L, int srate, int samples){
#else
    void initDSP(lua_State *L, int srate, int samples){
#endif
    luaL_openlibs(L);
    // version
    lua_pushnumber(L, VERSION);
    lua_setglobal(L, "VERSION");
    // srate
    lua_pushinteger(L, srate );
    lua_setglobal(L, "srate");
    // samples
    lua_pushinteger(L, samples );
    lua_setglobal(L, "samples");
    // rand
    lua_pushinteger(L, rand());
    lua_setglobal(L, "rand");
    // loop = { type=0, start=-1, stop=-1 }
    lua_newtable(L);
    lua_pushliteral( L, "type" );
    lua_pushinteger(L, 0);
    lua_settable(L,-3);
    lua_pushliteral( L, "start" );
    lua_pushinteger(L, -1);
    lua_settable(L,-3);
    lua_pushliteral( L, "stop" );
    lua_pushinteger(L, -1);
    lua_settable(L,-3);
    lua_setglobal(L,"loop");
}

#ifdef __cplusplus
  static inline void printError(lua_State *L){
#else      
  void printError(lua_State *L){
#endif
    puts(lua_tostring(L, lua_gettop(L)));
    lua_pop(L, lua_gettop(L));  
}

#ifdef __cplusplus
  static inline bool initSYN(lua_State *L, const char *fn){
#else      
  bool initSYN(lua_State *L, const char *fn){
#endif
    FILE *file;
    file = fopen(fn,"r");
    bool exist = (file != NULL);
    if( exist ) fclose(file);
    else{
        printf("LUA: writing %s\n",fn);
        file = fopen(fn,"w");
        fwrite(config_lua, 1 , config_lua_len, file );
        fclose(file);
    }
    if( luaL_dofile(L,fn) == LUA_OK ){
        lua_pop(L, lua_gettop(L));
        return true;
    }
    printError(L);
    return false;
}

#ifdef __cplusplus
};
#endif

#endif
