/******************************************************************************
 *
 * Project:  Shapelib
 * Purpose:  Sample application for dumping contents of a shapefile to
 *           the terminal in human readable form.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 *
 * SPDX-License-Identifier: MIT OR LGPL-2.0-or-later
 ******************************************************************************
 *
 */
// include the Defold SDK
#include <dmsdk/sdk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

void addBounds(lua_State *L, double *minb, double *maxb)
{
    int idx = 1;
    lua_newtable(L);
    lua_newtable(L);
    lua_pushnumber(L, minb[0]);
    lua_rawseti(L, -2, idx++);
    lua_pushnumber(L, minb[1]);
    lua_rawseti(L, -2, idx++);
    lua_pushnumber(L, minb[2]);
    lua_rawseti(L, -2, idx++);
    lua_pushnumber(L, minb[3]);
    lua_rawseti(L, -2, idx++);
    lua_setfield(L, -2, "min");

    idx = 0;
    lua_newtable(L);
    lua_pushnumber(L, maxb[0]);
    lua_rawseti(L, -2, idx++);
    lua_pushnumber(L, maxb[1]);
    lua_rawseti(L, -2, idx++);
    lua_pushnumber(L, maxb[2]);
    lua_rawseti(L, -2, idx++);
    lua_pushnumber(L, maxb[3]);
    lua_rawseti(L, -2, idx++);
    lua_setfield(L, -2, "max");

    lua_setfield(L, -2, "bounds");
}

int shpget(lua_State *L, bool bValidate, bool bHeaderOnly, int nPrecision, const char *shpfile)
{
    /* -------------------------------------------------------------------- */
    /*      Open the passed shapefile.                                      */
    /* -------------------------------------------------------------------- */
    SHPHandle hSHP = SHPOpen(shpfile, "rb");
    if (hSHP == NULL)
    {
        printf("Unable to open:%s\n", shpfile);
        exit(1);
    }

    lua_newtable(L);
    int idx = 1;
    
    /* -------------------------------------------------------------------- */
    /*      Print out the file bounds.                                      */
    /* -------------------------------------------------------------------- */
    int nEntities;
    int nShapeType;
    double adfMinBound[4];
    double adfMaxBound[4];
    SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

    //printf("Shapefile Type: %s   # of Shapes: %d\n\n", SHPTypeName(nShapeType),
    //       nEntities);

    lua_pushstring(L, SHPTypeName(nShapeType));
    lua_setfield(L, -2, "shapetype");
    lua_pushnumber(L, nEntities);
    lua_setfield(L, -2, "nEntities");

    //printf("File Bounds: (%.*g,%.*g,%.*g,%.*g)\n"
    //       "         to  (%.*g,%.*g,%.*g,%.*g)\n",
    //       nPrecision, adfMinBound[0], nPrecision, adfMinBound[1], nPrecision,
    //       adfMinBound[2], nPrecision, adfMinBound[3], nPrecision,
    //       adfMaxBound[0], nPrecision, adfMaxBound[1], nPrecision,
    //       adfMaxBound[2], nPrecision, adfMaxBound[3]);

    addBounds(L, adfMinBound, adfMaxBound);

    /* -------------------------------------------------------------------- */
    /*	Skim over the list of shapes, printing all the vertices.	*/
    /* -------------------------------------------------------------------- */
    int nInvalidCount = 0;

    lua_newtable(L);
    for (int i = 0; i < nEntities && !bHeaderOnly; i++)
    {
        SHPObject *psShape = SHPReadObject(hSHP, i);

        if (psShape == NULL)
        {
            fprintf(stderr, "Unable to read shape %d, terminating object reading.\n",i);
            break;
        }

        if (psShape->bMeasureIsUsed) {
            // printf("\nShape:%d (%s)  nVertices=%d, nParts=%d\n"
            //        "  Bounds:(%.*g,%.*g, %.*g, %.*g)\n"
            //        "      to (%.*g,%.*g, %.*g, %.*g)\n",
            //        i, SHPTypeName(psShape->nSHPType), psShape->nVertices,
            //        psShape->nParts, nPrecision, psShape->dfXMin, nPrecision,
            //        psShape->dfYMin, nPrecision, psShape->dfZMin, nPrecision,
            //        psShape->dfMMin, nPrecision, psShape->dfXMax, nPrecision,
            //        psShape->dfYMax, nPrecision, psShape->dfZMax, nPrecision,
            //        psShape->dfMMax);
            lua_pushstring(L, SHPTypeName(psShape->nSHPType));
            lua_setfield(L, -2, "shapetype");
            lua_pushnumber(L, psShape->nVertices);
            lua_setfield(L, -2, "nVertices");
            lua_pushnumber(L, psShape->nParts);
            lua_setfield(L, -2, "nParts");
            double minbound[] = { psShape->dfXMin, psShape->dfYMin, psShape->dfZMin, psShape->dfMMin };
            double maxbound[] = { psShape->dfXMax, psShape->dfYMax, psShape->dfZMax, psShape->dfMMax };
            addBounds(L, minbound, maxbound);
        }
        else  {
            // printf("\nShape:%d (%s)  nVertices=%d, nParts=%d\n"
            //        "  Bounds:(%.*g,%.*g, %.*g)\n"
            //        "      to (%.*g,%.*g, %.*g)\n",
            //        i, SHPTypeName(psShape->nSHPType), psShape->nVertices,
            //        psShape->nParts, nPrecision, psShape->dfXMin, nPrecision,
            //        psShape->dfYMin, nPrecision, psShape->dfZMin, nPrecision,
            //        psShape->dfXMax, nPrecision, psShape->dfYMax, nPrecision,
            //        psShape->dfZMax);
            lua_pushstring(L, SHPTypeName(psShape->nSHPType));
            lua_setfield(L, -2, "shapetype");
            lua_pushnumber(L, psShape->nVertices);
            lua_setfield(L, -2, "nVertices");
            lua_pushnumber(L, psShape->nParts);
            lua_setfield(L, -2, "nParts");
            double minbound[] = { psShape->dfXMin, psShape->dfYMin, psShape->dfZMin, 0.0 };
            double maxbound[] = { psShape->dfXMax, psShape->dfYMax, psShape->dfZMax, 0.0 };
            addBounds(L, minbound, maxbound);
        }

        if (psShape->nParts > 0 && psShape->panPartStart[0] != 0)
        {
            fprintf(stderr, "panPartStart[0] = %d, not zero as expected.\n", psShape->panPartStart[0]);
        }

        lua_newtable(L);
        int vidx = 1;
        for (int j = 0, iPart = 1; j < psShape->nVertices; j++)
        {
            const char *pszPartType = "";

            if (j == 0 && psShape->nParts > 0)
                pszPartType = SHPPartTypeName(psShape->panPartType[0]);

            const char *pszPlus;
            if (iPart < psShape->nParts && psShape->panPartStart[iPart] == j)
            {
                pszPartType = SHPPartTypeName(psShape->panPartType[iPart]);
                iPart++;
                pszPlus = "+";
            }
            else
                pszPlus = " ";

            if (psShape->bMeasureIsUsed) {
                // printf("   %s (%.*g,%.*g, %.*g, %.*g) %s \n", pszPlus,
                //        nPrecision, psShape->padfX[j], nPrecision,
                //        psShape->padfY[j], nPrecision, psShape->padfZ[j],
                //        nPrecision, psShape->padfM[j], pszPartType);
                lua_newtable(L);
                lua_pushnumber(L, psShape->padfX[j]);
                lua_rawseti(L, -2, 1); 
                lua_pushnumber(L, psShape->padfY[j]);
                lua_rawseti(L, -2, 1); 
                lua_pushnumber(L, psShape->padfZ[j]);
                lua_rawseti(L, -2, 1); 
                lua_pushnumber(L, psShape->padfM[j]);
                lua_rawseti(L, -2, 1); 

                lua_rawseti(L, -2, vidx++); 
            }
            else {
                // printf("   %s (%.*g,%.*g, %.*g) %s \n", pszPlus, nPrecision,
                //        psShape->padfX[j], nPrecision, psShape->padfY[j],
                //        nPrecision, psShape->padfZ[j], pszPartType);
                lua_newtable(L);
                lua_pushnumber(L, psShape->padfX[j]);
                lua_rawseti(L, -2, 1); 
                lua_pushnumber(L, psShape->padfY[j]);
                lua_rawseti(L, -2, 1); 
                lua_pushnumber(L, psShape->padfZ[j]);
                lua_rawseti(L, -2, 1); 

                lua_rawseti(L, -2, vidx++); 
            }
        }
        lua_setfield(L, -2, "verts");

        if (bValidate)
        {
            int nAltered = SHPRewindObject(hSHP, psShape);

            if (nAltered > 0)
            {
                printf("  %d rings wound in the wrong direction.\n", nAltered);
                nInvalidCount++;
            }
        }

        SHPDestroyObject(psShape);
    }
    lua_setfield(L, -2, "entities");

    SHPClose(hSHP);

    lua_rawseti(L, 3, 1); 

    if (bValidate)
    {
        printf("%d object has invalid ring orderings.\n", nInvalidCount);
    }

#ifdef USE_DBMALLOC
    malloc_dump(2);
#endif
    return 0;
}
