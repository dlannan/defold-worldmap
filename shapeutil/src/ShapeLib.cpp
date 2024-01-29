// ShapeLib.cpp
// Extension lib defines
#define LIB_NAME "ShapeUtil"
#define MODULE_NAME "shputil"

// include the Defold SDK
#include <dmsdk/sdk.h>

#include <vector>
#include <sstream>
#include <string>
#include <array>
#include <unordered_map>

#include "shapefil.h"
#include "triangle.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "stb_image_resize.h"

// ---------------------------------------------------------------------------------------
// Extern shape commands. Derived from the tools in ShapeLib
int shpdump(bool bValidate, bool bHeaderOnly, int nPrecision, const char *shpfile);
int shptreedump(int nExpandShapes, int nMaxDepth, const char *pszInputIndexFilename, const char *pszTargetFile);

int shpget(lua_State *L, bool bValidate, bool bHeaderOnly, int nPrecision, const char *shpfile);
int shptreeget(lua_State *L, const char *pszInputIndexFilename, const char *pszTargetFile);

const double UPSCALE = 1.0;
const double DOWNSCALE = 1 / 10000.0;

// ---------------------------------------------------------------------------------------
// List of polygons that have been converted from shape polygons

typedef struct std::vector<Point> polylist;
static std::vector<polylist>    polygons;

static int TriangulatePolygon(lua_State* L)
{
    // Incoming table is a poly - list of verts
    //DM_LUA_STACK_CHECK(L, 1);
    int polyid = (int)luaL_checknumber(L, 1);   // poly id (lookup into polygons)
    luaL_checktype(L, 2, LUA_TTABLE);   // indices table
    luaL_checktype(L, 3, LUA_TTABLE);   // verts table

    int indidx = (int)lua_objlen(L, 2)+1;
    int vertidx = (int)lua_objlen(L, 3)+1;

    polylist poly = polygons[polyid];    

    MooPolygon  moo;
    for(int i=0; i<poly.size(); i++)
        moo.push(poly[i]);

    printf("Points: %d\n", (int)poly.size()/2);

    std::vector<struct Point> res = moo.triangulate();

    printf("Triangles: %d\n",(int)res.size()/3);
 
    int tcount = res.size()/3;    

    if(tcount > 0) {
        printf("Indices: %d        Verts: %d\n", indidx, vertidx);
                
        for (int i=0; i<tcount; i++)
        {
            lua_pushnumber(L, i);
            lua_rawseti(L, 2, indidx++); 
        }
        
        for(std::size_t i = 0; i < res.size(); i++) 
        {
            lua_pushnumber(L, res[i].x * DOWNSCALE);
            lua_rawseti(L, 3, vertidx++); 
            lua_pushnumber(L, res[i].y * DOWNSCALE);
            lua_rawseti(L, 3, vertidx++); 
            lua_pushnumber(L, 0.0);
            lua_rawseti(L, 3, vertidx++); 
        }
    }

    lua_pushnumber(L, tcount);
    return 1;
}

typedef struct vec4 {
    union {
        unsigned char b[4];
        uint32_t val;
    };
} _vec4;

float step(float a, float x)
{
    return x >= a;
}

// vec4 encode32(float f) {
//     float e =5.0;

//     float F = abs(f); 
//     float Sign = step(0.0,-f);
//     float Exponent = floor(log2(F)); 
//     float Mantissa = (exp2(- Exponent) * F);
//     Exponent = floor(log2(F) + 127.0) + floor(log2(Mantissa));
//     vec4 rgba;
//     rgba.b[0] = (unsigned char)(128.0 * Sign  + floor(Exponent*exp2(-1.0)));
//     rgba.b[1] = (unsigned char)(128.0 * fmod(Exponent, 2.0) + fmod(floor(Mantissa*128.0), 128.0));  
//     rgba.b[2] = (unsigned char)(floor( fmod(floor(Mantissa*exp2(23.0 -8.0)), exp2(8.0))));
//     rgba.b[3] = (unsigned char)(floor(exp2(23.0)* fmod(Mantissa, exp2(-15.0))));
//     return rgba;
// }

float fract(float x)
{
    return(x - floor(x));
}

float shift_right (float v, float amt) { 
    v = floor(v) + 0.5; 
    return floor(v / exp2(amt)); 
}

float shift_left (float v, float amt) { 
    return floor(v * exp2(amt) + 0.5); 
}

float mask_last (float v, float bits) { 
    return fmod(v, shift_left(1.0, bits)); 
}

float extract_bits (float num, float from, float to) { 
    from = floor(from + 0.5); to = floor(to + 0.5); 
    return mask_last(shift_right(num, from), to - from); 
}

// uint32_t encode32 (float val) { 
//     if (val == 0.0) return 0; 
//     float sign = val > 0.0 ? 0.0 : 1.0; 
//     val = fabs(val); 
//     float exponent = floor(log2(val)); 
//     float biased_exponent = exponent + 127.0; 
//     float fraction = ((val / exp2(exponent)) - 1.0) * 8388608.0; 
//     float t = biased_exponent / 2.0; 
//     float last_bit_of_biased_exponent = fract(t) * 2.0; 
//     float remaining_bits_of_biased_exponent = floor(t); 
//     vec4 rgba;
//     rgba.b[0] = (unsigned char)extract_bits(fraction, 0.0, 8.0);// / 255.0; 
//     rgba.b[1] = (unsigned char)extract_bits(fraction, 8.0, 16.0);// / 255.0; 
//     rgba.b[2] = (unsigned char)(last_bit_of_biased_exponent * 128.0 + extract_bits(fraction, 16.0, 23.0));// / 255.0; 
//     rgba.b[3] = (unsigned char)(sign * 128.0 + remaining_bits_of_biased_exponent);// / 255.0; 
//     return rgba.val; 
// }

int32_t encode32 (float val) { 
    return (int32_t)(val * 10000.0);
}

static int savepolystoimage( lua_State *L )
{
    DM_LUA_STACK_CHECK(L, 0);
    const char * filename = luaL_checkstring(L, 1);
    int polyid = luaL_checkinteger(L, 2);
    int dim = luaL_checkinteger(L, 3);

    // calc total values needed
    int vertcount = 1;
    for (int p; p<polygons.size(); p++) {
        vertcount++;
        vertcount+= polygons[p].size() * 2;
    }
    printf("vertcount: %d   dim * dim: %d\n", vertcount, dim * dim);
    while(vertcount > dim * dim) {
        printf("Dim too small. Upsizing from: %d to %d\n", dim, dim + dim);
        dim = dim + dim;
    }

    int imgsize = dim * dim;
    int32_t *pdata = (int32_t *)malloc( imgsize * 4 );
    memset(pdata, 0, imgsize * 4 );

    int i = 0;
    pdata[i++] = encode32((float)polygons.size());
    
    for (int p=0; p<1; p++) {

        polylist poly = polygons[p];
        // Num verts in poly
        pdata[i++] = encode32((float)poly.size());
        printf("Poly: %d  Size: %d\n", p, (int)poly.size());

        for(int q=0; q<poly.size(); q++)
        {
            printf("X: %f  Y: %f\n", poly[q].x, poly[q].y);
            pdata[i++] = encode32(poly[q].x);
            pdata[i++] = encode32(poly[q].y);
            //pdata[i++] = 0.0f;
        }
    }

    // if CHANNEL_NUM is 4, you can use alpha channel in png
    stbi_write_png(filename, dim, dim, STBI_rgb_alpha, (unsigned char *)pdata, dim * 4);
    free(pdata);

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

static int ShapeSubmitPolygon(lua_State *L)
{
    // List of verts for a polygon
    luaL_checktype(L, 1, LUA_TTABLE);

    int vertcount = (int)lua_objlen(L, 1);
    polylist poly;

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
        
        poly.push_back( Point(x * UPSCALE, y * UPSCALE) );
    }

    poly.clear();
    poly.push_back(Point(.77, 0.0));
    poly.push_back(Point(0.15, 0.0625));
    poly.push_back(Point(-0.5, 0.0625-0.7));
    poly.push_back(Point(-0.63, 0.0));
    poly.push_back(Point(-0.15, -0.0625));
    poly.push_back(Point(0.5, -0.0625));

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
    {"polysave", savepolystoimage },
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
