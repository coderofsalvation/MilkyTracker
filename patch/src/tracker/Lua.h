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

#include "../lua/minilua.h"

#if defined(_WIN32)
#include <cstdlib>
#else
#include <math.h>
#endif

extern unsigned char config_lua[];
extern unsigned int config_lua_len;

#define VERSION 0.1
        
#ifdef __cplusplus
class Lua {

    public:

#endif

#ifdef __cplusplus
    static inline int getTableInt( lua_State *L, const char *table, const char *key, int fallback){
#else 
    int getTableInt( lua_State *L, const char *table, const char *key, int fallback){
#endif
    int v = fallback;
    lua_getglobal(L, table);
    if (lua_istable(L, -1))  {
        lua_getfield(L, -1, key);
        v = lua_tonumber(L, -1);
    }
    return v;
}

#ifdef __cplusplus
    static inline char * getTableString( lua_State *L, const char *table, const char *key, char *fallback){
#else 
    char * getTableString( lua_State *L, const char *table, const char *key, char *fallback){
#endif
    char *s = fallback;
    lua_getglobal(L, table);
    if (lua_istable(L, -1))  {
        lua_getfield(L, -1, key);
        s = (char *)lua_tostring(L, -1);
    }
    return s;
}

#ifdef __cplusplus
    static inline float getNumber( lua_State *L, const char *key, float fallback){
#else 
    char * getNumber( lua_State *L, const char *key, float fallback){
#endif
    float v = fallback;
    lua_getfield(L, -2, key);
    if( lua_isnumber(L,-1) ){
        v = lua_tonumber(L,-1);
        lua_pop(L,1);
    }
    return v;
}

#ifdef __cplusplus
    static inline char * getString( lua_State *L, const char *key, char *fallback){
#else 
    char * getString( lua_State *L, const char *key, const char *key, char *fallback){
#endif
    char *v = fallback;
    lua_getfield(L, -2, key);
    if( lua_isstring(L,-1) ){
        v = (char *)lua_tostring(L,-1);
        lua_pop(L,1);
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
    //// loop = { type=0, start=-1, stop=-1 }
    //lua_newtable(L);
    //lua_pushliteral( L, "type" );
    //lua_pushinteger(L, 0);
    //lua_settable(L,-3);
    //lua_pushliteral( L, "start" );
    //lua_pushinteger(L, -1);
    //lua_settable(L,-3);
    //lua_pushliteral( L, "stop" );
    //lua_pushinteger(L, -1);
    //lua_settable(L,-3);
    //lua_setglobal(L,"loop");
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

/*
 * macro's to make code less verbose 
 * ps. the dowhile's are needed to ensure proper macro-expansion
 */

#define LUA_FOREACH(key) \
    lua_getglobal(L, key);\
    if (lua_istable(L, -1))  {\
        lua_pushvalue(L, -1);\
        lua_pushnil(L);\
        while (lua_next(L, -2)){\
            lua_pushvalue(L, -2);\

#define LUA_FOREACH_END \
			lua_pop(L, 2);\
        }\
        lua_pop(L, 1);\
    }
    
#endif
