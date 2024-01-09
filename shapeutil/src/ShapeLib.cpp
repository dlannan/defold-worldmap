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


int shpdump(bool bValidate, bool bHeaderOnly, int nPrecision, const char *shpfile);
int shptreedump(int nExpandShapes, int nMaxDepth, const char *pszInputIndexFilename, const char *pszTargetFile);

int shpget(lua_State *L, bool bValidate, bool bHeaderOnly, int nPrecision, const char *shpfile);
int shptreeget(lua_State *L, const char *pszInputIndexFilename, const char *pszTargetFile);

// Triangulation results - can store multiple if needed
static std::vector<Vector2dVector>  triresults;

static int TriangulatePolygon(lua_State* L)
{
    // Incoming table is a poly - list of verts
    DM_LUA_STACK_CHECK(L, 0);
    luaL_checktype(L, 1, LUA_TTABLE);
    return 0;
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

typedef struct polypt {
    double X;
    double Y;
    polypt(double _X, double _Y): X(_X), Y(_Y) {}
} _polypt;

typedef struct polygon {
    std::vector<polypt>        pts;
} _polygon;

static std::vector<polygon>    polygons;

static GLuint VertexArrayID;

static int ShapeRenderPolygon(lua_State *L)
{
    glColor3f(1.0f, 1.0f, 1.0f);
    for( int i=0;i < polygons.size();i++)
    {
        glBegin(GL_LINE_LOOP);
        for(int j=0;j <polygons[i].pts.size();j++)
        {
            glVertex2f(polygons[i].pts[j].X,polygons[i].pts[j].Y);
        }
        glEnd();
    }
    return 0;
}

static int ShapeSubmitPolygon(lua_State *L)
{
    // List of verts for a polygon
    luaL_checktype(L, 1, LUA_TTABLE);

    polygon poly;
    
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

        poly.pts.push_back(polypt(x,y));
    }
    
    polygons.push_back(poly);
    return 0;
}

static int ShapeClearPolygons(lua_State *L)
{
    if(polygons.size() == 0) return 0;
    for( int i=0;i < polygons.size();i++)
    {
        polygons[i].pts.clear();
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
    {"polysrender", ShapeRenderPolygon },
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
