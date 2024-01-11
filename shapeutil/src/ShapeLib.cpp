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
#include "triangulate.h"


// ---------------------------------------------------------------------------------------
// Extern shape commands. Derived from the tools in ShapeLib
int shpdump(bool bValidate, bool bHeaderOnly, int nPrecision, const char *shpfile);
int shptreedump(int nExpandShapes, int nMaxDepth, const char *pszInputIndexFilename, const char *pszTargetFile);

int shpget(lua_State *L, bool bValidate, bool bHeaderOnly, int nPrecision, const char *shpfile);
int shptreeget(lua_State *L, const char *pszInputIndexFilename, const char *pszTargetFile);

// ---------------------------------------------------------------------------------------
// List of polygons that have been converted from shape polygons

static std::vector<Vector2dVector>    polygons;

static int TriangulatePolygon(lua_State* L)
{
    // Incoming table is a poly - list of verts
    DM_LUA_STACK_CHECK(L, 1);
    int polyid = (int)luaL_checknumber(L, 1);   // poly id (lookup into polygons)
    luaL_checktype(L, 2, LUA_TTABLE);   // indices table
    luaL_checktype(L, 3, LUA_TTABLE);   // verts table

    int indidx = (int)lua_objlen(L, 2)+1;
    int vertidx = (int)lua_objlen(L, 3)+1;

    Vector2dVector poly = polygons[polyid];
    // Vector2dVector a;

    // a.push_back( Vector2d(0,6));
    // a.push_back( Vector2d(0,0));
    // a.push_back( Vector2d(3,0));
    // a.push_back( Vector2d(4,1));
    // a.push_back( Vector2d(6,1));
    // a.push_back( Vector2d(8,0));
    // a.push_back( Vector2d(12,0));
    // a.push_back( Vector2d(13,2));
    // a.push_back( Vector2d(8,2));
    // a.push_back( Vector2d(8,4));
    // a.push_back( Vector2d(11,4));
    // a.push_back( Vector2d(11,6));
    // a.push_back( Vector2d(6,6));
    // a.push_back( Vector2d(4,3));
    // a.push_back( Vector2d(2,6));

    // allocate an STL vector to hold the answer.

    Vector2dVector result;

    //  Invoke the triangulator to triangulate this polygon.
    Triangulate::Process(poly,result);

    int tcount = poly.size();

    if(tcount > 0) {
        printf("Indices: %d        Verts: %d\n", indidx, vertidx);
                
        for (int i=0; i<tcount; i++)
        {
            lua_pushnumber(L, i);
            lua_rawseti(L, 2, indidx++); 
        }
        
        for (int i=0; i<tcount; i++)
        {
            lua_pushnumber(L, poly[i].X);
            lua_rawseti(L, 3, vertidx++); 
            lua_pushnumber(L, poly[i].Y);
            lua_rawseti(L, 3, vertidx++); 
            lua_pushnumber(L, 0.0);
            lua_rawseti(L, 3, vertidx++); 
        }
    }
    lua_pushnumber(L, tcount);
    return 1;
}

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

void print_table(lua_State *L, int tab)
{
    /* table is in the stack at index 't' */
    lua_pushnil(L);  /* first key */
    while (lua_next(L, -2) != 0) {
        for(int i = 0; i<tab; i++)
            printf("\t");
        
        if (lua_type(L, -2)==LUA_TSTRING)
        {
            const char *key = lua_tostring(L, -2);
            
            if(lua_isstring(L, -1))
                printf("%s = %s\n", key, lua_tostring(L, -1));
            else if(lua_isnumber(L, -1))
                printf("%s = %f\n", key, lua_tonumber(L, -1));
            else if(lua_istable(L, -1)) {
                printf("%s\n", key);
                print_table(L, tab+1);
            }
        } 
        else if (lua_type(L, -2)==LUA_TNUMBER)
        {
            int key = lua_tonumber(L, -2);

            if(lua_isstring(L, -1))
                printf("%d = %s\n", key, lua_tostring(L, -1));
            else if(lua_isnumber(L, -1))
                printf("%d = %f\n", key, lua_tonumber(L, -1));
            else if(lua_istable(L, -1)) {
                printf("%d\n", key);
                print_table(L, tab+1);
            }
        }         
           
        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
    }
}

static int PrintTable(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    print_table(L, 1);
    return 0;
}

static int ShapeSubmitPolygon(lua_State *L)
{
    // List of verts for a polygon
    luaL_checktype(L, 1, LUA_TTABLE);

    Vector2dVector poly;
    int vertcount = (int)lua_objlen(L, 1);

    int i = 0;
    // Table of list of verts
    lua_pushnil(L);  /* first key */
    while(lua_next(L, -2) != 0) {

        int key = lua_tonumber(L, -2);
        //printf("key: %d\n", key);
        
        // Table with two verts in it.
        lua_getfield(L, -1, "X");
        double x = lua_tonumber(L, -1);
        //printf("x: %f\n", x);
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "Y");
        double y = lua_tonumber(L, -1);
        //printf("y: %f\n", y);
        lua_pop(L, 1);

        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
        poly.push_back(Vector2d(x,y));
    }

    // if(poly.size() > 0) poly.push_back(poly[0]);
    
    polygons.push_back(poly);
    lua_pushnumber(L, polygons.size()-1);
    return 1;
}

static int ShapeClearPolygons(lua_State *L)
{
    if(polygons.size() == 0) return 0;
    for( int i=0;i < polygons.size();i++)
    {
        polygons[i].clear();
    }
    polygons.clear();
    return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"dump", ShapeDump },
    {"treedump", ShapeTreeDump },
    {"load", ShapeGet },
    {"treeload", ShapeTreeGet },
    {"polysubmit", ShapeSubmitPolygon },
    {"polysclear", ShapeClearPolygons },
    {"polygontri", TriangulatePolygon },
    {"print", PrintTable },
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
