// myextension.cpp
// Extension lib defines
#define LIB_NAME "GeomExtension"
#define MODULE_NAME "geomextension"

// include the Defold SDK
#include <dmsdk/sdk.h>
#include <stdlib.h>

static int SetBufferIntsFromTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    size_t offset = luaL_checknumber(L, 1);
    size_t length = luaL_checknumber(L, 2);
    const unsigned char *data = (unsigned char *)luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    
    // Now we have the data, cast it to the union and write back out.
    int idx = 1;
    for( int i=0; i<length; i+=sizeof(unsigned short))
    {
        unsigned int val = ((unsigned int)data[i+1+offset] << 8) | ((unsigned int)data[i+offset]);
        //printf("%d\n", val);
        lua_pushnumber(L, val);  /* value */
        lua_rawseti(L, 4, idx++);  /* set table at key `i' */
    }

    return 0;
}

static int SetBufferFloatsFromTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    size_t offset = luaL_checknumber(L, 1);
    size_t length = luaL_checknumber(L, 2);
    const char *data = luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);
    
    // Now we have the data, cast it to the union and write back out.
    int idx = 1;
    for( int i=0; i<length; i+= sizeof(float))
    {
        float val = *(float *)(data + i + offset);
        //printf("%f\n", val);
        lua_pushnumber(L, val);  /* value */
        lua_rawseti(L, 4, idx++);  /* set table at key `i' */
    }

    return 0;
}

static void GetTableNumbersInt( lua_State * L, int tblidx, int *data )
{
    // Iterate indices and set float buffer with correct lookups
    lua_pushnil(L);
    size_t idx = 0;
    // Build a number array matching the buffer. They are all assumed to be type float (for the time being)
    while( lua_next( L, tblidx ) != 0) {
        data[idx++] = (int)lua_tonumber( L, -1 );
        lua_pop( L, 1 );
    }
}

static void GetTableNumbersFloat( lua_State * L, int tblidx, float *data )
{
    // Iterate indices and set float buffer with correct lookups
    lua_pushnil(L);
    size_t idx = 0;
    // Build a number array matching the buffer. They are all assumed to be type float (for the time being)
    while( lua_next( L, tblidx ) != 0) {
        data[idx++] = lua_tonumber( L, -1 );
        lua_pop( L, 1 );
    }
}

static int SetBufferBytesFromTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmScript::LuaHBuffer *buffer = dmScript::CheckBuffer(L, 1);
    const char *streamname = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);

    float* bytes = 0x0;
    uint32_t count = 0;
    uint32_t components = 0;
    uint32_t stride = 0;
    dmBuffer::Result r = dmBuffer::GetStream(buffer->m_Buffer, dmHashString64(streamname), (void**)&bytes, &count, &components, &stride);

    if(components == 0 || count == 0) return 0;

    // This is very rudimentary.. will make nice later (maybe)    
    size_t indiceslen = lua_objlen(L, 3);
    int * idata = (int *)calloc(indiceslen, sizeof(int)); 
    GetTableNumbersInt(L, 3, idata);

    size_t floatslen = lua_objlen(L, 4);
    float *floatdata = (float *)calloc(floatslen, sizeof(float));    
    GetTableNumbersFloat(L, 4, floatdata);

    printf("count: %d  indices: %d\n", count, (int)indiceslen);
    printf("total: %d  floatlen: %d\n", count * components, (int)floatslen);

    if (r == dmBuffer::RESULT_OK) {
        for (int i = 0; i < count; ++i)
        {
            for (int c = 0; c < components; ++c)
            {
                // printf("i: %d  idata[i]: %d   off: %d\n", i, idata[i], idata[i] * components + c);
                bytes[c] = floatdata[idata[i] * components + c];
            }
            bytes += stride;
        }
    } else {
        // handle error
    }
    
    free(floatdata);
    free(idata);
    r = dmBuffer::ValidateBuffer(buffer->m_Buffer);
    return 0;
}


static int SetBufferBytes(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmScript::LuaHBuffer *buffer = dmScript::CheckBuffer(L, 1);
    const char *streamname = luaL_checkstring(L, 2);
    const char *bufferstring = luaL_checkstring(L, 3);
    
    uint8_t* bytes = 0x0;
    uint32_t size = 0;
    uint32_t count = 0;
    uint32_t components = 0;
    uint32_t stride = 0;
    dmBuffer::Result r = dmBuffer::GetStream(buffer->m_Buffer, dmHashString64(streamname), (void**)&bytes, &count, &components, &stride);

    size_t idx = 0;
    if (r == dmBuffer::RESULT_OK) {
        for (int i = 0; i < count; ++i)
        {
            for (int c = 0; c < components; ++c)
            {
                bytes[c] = bufferstring[idx++];
            }
            bytes += stride;
        }
    } else {
        // handle error
    }
        
    r = dmBuffer::ValidateBuffer(buffer->m_Buffer);
    return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"setbufferbytes", SetBufferBytes},
    {"setbufferbytesfromtable", SetBufferBytesFromTable},
    {"setbufferfloatsfromtable", SetBufferFloatsFromTable},
    {"setbufferintsfromtable", SetBufferIntsFromTable},
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

dmExtension::Result AppInitializeGeomExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppInitializeGeomExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeGeomExtension(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeGeomExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppFinalizeGeomExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeGeomExtension(dmExtension::Params* params)
{
    dmLogInfo("FinalizeGeomExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result OnUpdateGeomExtension(dmExtension::Params* params)
{
    // dmLogInfo("OnUpdateGeomExtension\n");
    return dmExtension::RESULT_OK;
}

void OnEventGeomExtension(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch(event->m_Event)
    {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            dmLogInfo("OnEventGeomExtension - EVENT_ID_ACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_DEACTIVATEAPP:
            dmLogInfo("OnEventGeomExtension - EVENT_ID_DEACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_ICONIFYAPP:
            dmLogInfo("OnEventGeomExtension - EVENT_ID_ICONIFYAPP\n");
            break;
        case dmExtension::EVENT_ID_DEICONIFYAPP:
            dmLogInfo("OnEventGeomExtension - EVENT_ID_DEICONIFYAPP\n");
            break;
        default:
            dmLogWarning("OnEventGeomExtension - Unknown event id\n");
            break;
    }
}

// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

// GeomExtension is the C++ symbol that holds all relevant extension data.
// It must match the name field in the `ext.manifest`
DM_DECLARE_EXTENSION(GeomExtension, LIB_NAME, AppInitializeGeomExtension, AppFinalizeGeomExtension, InitializeGeomExtension, OnUpdateGeomExtension, OnEventGeomExtension, FinalizeGeomExtension)
