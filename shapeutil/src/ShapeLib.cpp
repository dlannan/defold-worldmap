// ShapeLib.cpp
// Extension lib defines
#define LIB_NAME "ShapeUtil"
#define MODULE_NAME "shputil"

// include the Defold SDK
#include <dmsdk/sdk.h>

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

int shpdump(bool bValidate, bool bHeaderOnly, int nPrecision, const char *shpfile);
int shptreedump(int nExpandShapes, int nMaxDepth, const char *pszInputIndexFilename, const char *pszTargetFile);

int shpget(lua_State *L, bool bValidate, bool bHeaderOnly, int nPrecision, const char *shpfile);
int shptreeget(lua_State *L, const char *pszInputIndexFilename, const char *pszTargetFile);

static int ShapeDump(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    char *shpfile = (char *)luaL_checkstring(L, 1);
    shpdump(false, false, 1, shpfile);
    return 0;
}

static int ShapeGet(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    char *shpfile = (char *)luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    shpget(L, false, false, 1, shpfile);
    return 0;
}

static int ShapeTreeDump(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int expand = (int)luaL_checknumber(L, 1);
    int maxdepth = (int)luaL_checknumber(L, 2);
    
    char *shpfileidx = (char *)luaL_checkstring(L, 3);
    char *shpfile = (char *)luaL_checkstring(L, 4);
    shptreedump(expand, maxdepth, shpfileidx, shpfile);
    return 0;
}

static int ShapeTreeGet(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    char *shpfileidx = (char *)luaL_checkstring(L, 1);
    char *shpfile = (char *)luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    
    shptreeget(L, shpfileidx, shpfile);
    return 0;
}


// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"dump", ShapeDump },
    {"treedump", ShapeTreeDump },
    {"load", ShapeGet },
    {"treeload", ShapeTreeGet },
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result AppInitializeMyExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppInitializeMyExtension");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeMyExtension(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeMyExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppFinalizeMyExtension");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeMyExtension(dmExtension::Params* params)
{
    dmLogInfo("FinalizeMyExtension");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result OnUpdateMyExtension(dmExtension::Params* params)
{
    // dmLogInfo("OnUpdateMyExtension");
    return dmExtension::RESULT_OK;
}

static void OnEventMyExtension(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch(event->m_Event)
    {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_ACTIVATEAPP");
            break;
        case dmExtension::EVENT_ID_DEACTIVATEAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_DEACTIVATEAPP");
            break;
        case dmExtension::EVENT_ID_ICONIFYAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_ICONIFYAPP");
            break;
        case dmExtension::EVENT_ID_DEICONIFYAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_DEICONIFYAPP");
            break;
        default:
            dmLogWarning("OnEventMyExtension - Unknown event id");
            break;
    }
}

// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

// MyExtension is the C++ symbol that holds all relevant extension data.
// It must match the name field in the `ext.manifest`
DM_DECLARE_EXTENSION(ShapeUtil, LIB_NAME, AppInitializeMyExtension, AppFinalizeMyExtension, InitializeMyExtension, OnUpdateMyExtension, OnEventMyExtension, FinalizeMyExtension)