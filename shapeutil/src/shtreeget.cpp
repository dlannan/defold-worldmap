/******************************************************************************
 *
 * Project:  Shapelib
 * Purpose:  Mainline for creating and dumping an ASCII representation of
 *           a quadtree.
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

#include "shapefil.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void SHPTreeNodeGet(lua_State *L, SHPTree *, SHPTreeNode *, const char *, int);
static void SHPTreeNodeSearchAndDump(SHPTree *, double *, double *);

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf("shptreedump [-maxdepth n] [-search xmin ymin xmax ymax]\n"
           "            [-v] [-o indexfilename] [-i indexfilename]\n"
           "            shp_file\n");
}

/************************************************************************/
/*                                main()                                */
/*   Assumes that the output table is at index 3 in luastate            */
/************************************************************************/
int shptreeget(lua_State *L, const char *pszInputIndexFilename, const char *pszTargetFile)
{
    int nMaxDepth = 0;
    int nExpandShapes = 1;
    bool bDoSearch = false;
    double adfSearchMin[4];
    double adfSearchMax[4];
    const char *pszOutputIndexFilename = NULL;

    // /* -------------------------------------------------------------------- */
    // /*      Do a search with an existing index file?                        */
    // /* -------------------------------------------------------------------- */
    // if (bDoSearch && pszInputIndexFilename != NULL)
    // {
    //     FILE *fp = fopen(pszInputIndexFilename, "rb");

    //     if (fp == NULL)
    //     {
    //         perror(pszInputIndexFilename);
    //         exit(1);
    //     }

    //     int nResultCount = 0;
    //     int *panResult =
    //         SHPSearchDiskTree(fp, adfSearchMin, adfSearchMax, &nResultCount);

    //     printf("Result: ");
    //     for (int iResult = 0; iResult < nResultCount; iResult++)
    //         printf("%d ", panResult[iResult]);
    //     printf("\n");
    //     free(panResult);

    //     fclose(fp);

    //     return 0;
    // }

    /* -------------------------------------------------------------------- */
    /*      Display a usage message.                                        */
    /* -------------------------------------------------------------------- */
    if (pszTargetFile == NULL)
    {
        Usage();
        return 1;
    }

    /* -------------------------------------------------------------------- */
    /*      Open the passed shapefile.                                      */
    /* -------------------------------------------------------------------- */
    SHPHandle hSHP = SHPOpen(pszTargetFile, "rb");

    if (hSHP == NULL)
    {
        printf("Unable to open:%s\n", pszTargetFile);
        return 1;
    }

    /* -------------------------------------------------------------------- */
    /*      Build a quadtree structure for this file.                       */
    /* -------------------------------------------------------------------- */
    SHPTree *psTree = SHPCreateTree(hSHP, 2, nMaxDepth, NULL, NULL);

    /* -------------------------------------------------------------------- */
    /*      Trim unused nodes from the tree.                                */
    /* -------------------------------------------------------------------- */
    SHPTreeTrimExtraNodes(psTree);

    // /* -------------------------------------------------------------------- */
    // /*      Dump tree to .qix file.                                         */
    // /* -------------------------------------------------------------------- */
    // if (pszOutputIndexFilename != NULL)
    // {
    //     SHPWriteTree(psTree, pszOutputIndexFilename);
    // }

    /* -------------------------------------------------------------------- */
    /*      Dump tree by recursive descent.                                 */
    /* -------------------------------------------------------------------- */
    // if (!bDoSearch)

        SHPTreeNodeGet(L, psTree, psTree->psRoot, "", nExpandShapes);
        lua_rawseti(L, 3, 1); 
        
    // /* -------------------------------------------------------------------- */
    // /*      or do a search instead.                                         */
    // /* -------------------------------------------------------------------- */
    // else
    //     SHPTreeNodeSearchAndDump(psTree, adfSearchMin, adfSearchMax);

    /* -------------------------------------------------------------------- */
    /*      cleanup                                                         */
    /* -------------------------------------------------------------------- */
    SHPDestroyTree(psTree);

    SHPClose(hSHP);

#ifdef USE_DBMALLOC
    malloc_dump(2);
#endif

    return 0;
}

/************************************************************************/
/*                           EmitCoordinate()                           */
/************************************************************************/

static void EmitCoordinate(lua_State *L, double *padfCoord, int nDimension)

{
    // const char *pszFormat;

    // if (fabs(padfCoord[0]) < 180 && fabs(padfCoord[1]) < 180)
    //     pszFormat = "%.9f";
    // else
    //     pszFormat = "%.2f";

    lua_newtable(L);
    int idx = 1;

    //printf("%s( SHPTreeNode\n", pszPrefix);
    lua_pushnumber(L, padfCoord[0]);
    lua_rawseti(L, -2, idx++);
    lua_pushnumber(L, padfCoord[1]);
    lua_rawseti(L, -2, idx++);
        
        
    //printf(pszFormat, padfCoord[0]);
    //printf(",");
    //printf(pszFormat, padfCoord[1]);

    if (nDimension > 2)
    {
        //printf(",");
        //printf(pszFormat, padfCoord[2]);
        lua_pushnumber(L, padfCoord[2]);
        lua_rawseti(L, -2, idx++);
    }
    if (nDimension > 3)
    {
        //printf(",");
        //printf(pszFormat, padfCoord[3]);
        lua_pushnumber(L, padfCoord[3]);
        lua_rawseti(L, -2, idx++);
    }
}

/************************************************************************/
/*                             EmitShape()                              */
/************************************************************************/

static void EmitShape(lua_State *L, SHPObject *psObject, const char *pszPrefix,
                      int nDimension)

{
    //printf("%s( Shape\n", pszPrefix);
    //printf("%s  ShapeId = %d\n", pszPrefix, psObject->nShapeId);
    lua_newtable(L);
    int idx = 1;
    
    //printf("%s  Min = (", pszPrefix);
    EmitCoordinate(L, &(psObject->dfXMin), nDimension);
    lua_setfield(L, -2, "min");
    //printf(")\n");

    //printf("%s  Max = (", pszPrefix);
    EmitCoordinate(L, &(psObject->dfXMax), nDimension);
    lua_setfield(L, -2, "max");
    //printf(")\n");

    lua_pushnumber(L, psObject->nVertices);
    lua_setfield(L, -2, "nVertices");
    lua_newtable(L);

    for (int i = 0; i < psObject->nVertices; i++)
    {
        double adfVertex[4];

        //printf("%s  Vertex[%d] = (", pszPrefix, i);
        
        adfVertex[0] = psObject->padfX[i];
        adfVertex[1] = psObject->padfY[i];
        adfVertex[2] = psObject->padfZ[i];
        adfVertex[3] = psObject->padfM[i];

        EmitCoordinate(L, adfVertex, nDimension);
        lua_rawseti(L, -2, i+1);
        //printf(")\n");
    }
    lua_setfield(L, -2, "verts");

    lua_pushnumber(L,  psObject->nParts);
    lua_setfield(L, -2, "ringcount");
    
    lua_newtable(L);
    int pidx = 1;
    for (int i = 0; i < psObject->nParts; i++)
    {
        lua_pushnumber(L, psObject->panPartStart[i]);
        lua_rawseti(L, -2, pidx++);
        if(i < psObject->nParts-1) {
            lua_pushnumber(L, psObject->panPartStart[i+1] - psObject->panPartStart[i]);
            lua_rawseti(L, -2, pidx++);
        } else {
            lua_pushnumber(L, psObject->nParts - psObject->panPartStart[i]);
            lua_rawseti(L, -2, pidx++);
        }            
    }
    lua_setfield(L, -2, "rings");
        
    //printf("%s)\n", pszPrefix);
}

/************************************************************************/
/*                          SHPTreeNodeDump()                           */
/*                                                                      */
/*      Dump a tree node in a readable form.                            */
/************************************************************************/

static void SHPTreeNodeGet(lua_State *L, SHPTree *psTree, SHPTreeNode *psTreeNode,
                            const char *pszPrefix, int nExpandShapes)

{
    char szNextPrefix[150];

    strcpy(szNextPrefix, pszPrefix);
    if (strlen(pszPrefix) < sizeof(szNextPrefix) - 3)
        strcat(szNextPrefix, "  ");

    // Main table is at index 3 on the lua State - this is target table to fill.
    // Make a new table for each node.    

    lua_newtable(L);
    int idx = 1;
    
    //printf("%s( SHPTreeNode\n", pszPrefix);
    lua_pushstring(L, pszPrefix);
    lua_setfield(L, -2, "id");
    
    /* -------------------------------------------------------------------- */
    /*      Emit the bounds.                                                */
    /* -------------------------------------------------------------------- */
    //printf("%s  Min = (", pszPrefix);
    //lua_pushliteral(L,"Min");
    EmitCoordinate(L, psTreeNode->adfBoundsMin, psTree->nDimension);
    lua_setfield(L, -2, "min");
    
    //printf(")\n");

    //printf("%s  Max = (", pszPrefix);
    //lua_pushliteral(L,"Max");
    EmitCoordinate(L, psTreeNode->adfBoundsMax, psTree->nDimension);
    lua_setfield(L, -2, "max");
        //printf(")\n");

    /* -------------------------------------------------------------------- */
    /*      Emit the list of shapes on this node.                           */
    /* -------------------------------------------------------------------- */
    if (nExpandShapes)
    {
        lua_newtable(L);
        //printf("%s  Shapes(%d):\n", pszPrefix, psTreeNode->nShapeCount);
        for (int i = 0; i < psTreeNode->nShapeCount; i++)
        {
            lua_newtable(L);
            SHPObject *psObject;
            psObject = SHPReadObject(psTree->hSHP, psTreeNode->panShapeIds[i]);
            assert(psObject != NULL);
            if (psObject != NULL)
            {
                lua_pushnumber(L, psObject->nSHPType);
                lua_setfield(L, -2, "shapetype");
                
                EmitShape(L, psObject, szNextPrefix, psTree->nDimension);
                lua_setfield(L, -2, "shape");
            }
            lua_rawseti(L, -2, i+1); 

            SHPDestroyObject(psObject);
        }
        lua_setfield(L, -2, "shapes");
    }
    else
    {
        //printf("%s  Shapes(%d): ", pszPrefix, psTreeNode->nShapeCount);
        for (int i = 0; i < psTreeNode->nShapeCount; i++)
        {
            //printf("%d ", psTreeNode->panShapeIds[i]);
        }
        //printf("\n");
    }

    /* -------------------------------------------------------------------- */
    /*      Emit subnodes.                                                  */
    /* -------------------------------------------------------------------- */
    if(psTreeNode->nSubNodes > 0) {
        lua_newtable(L);    
        for (int i = 0; i < psTreeNode->nSubNodes; i++)
        {
            if (psTreeNode->apsSubNode[i] != NULL) {
                SHPTreeNodeGet(L,psTree, psTreeNode->apsSubNode[i], szNextPrefix, nExpandShapes);
                lua_rawseti(L, -2, i+1);
            }
        }
        lua_setfield(L, -2, "nodes");
    }

    //printf("%s)\n", pszPrefix);
    return;
}

/************************************************************************/
/*                      SHPTreeNodeSearchAndDump()                      */
/************************************************************************/

static void SHPTreeNodeSearchAndDump(SHPTree *hTree, double *padfBoundsMin,
                                     double *padfBoundsMax)

{
    /* -------------------------------------------------------------------- */
    /*      Perform the search for likely candidates.  These are shapes     */
    /*      that fall into a tree node whose bounding box intersects our    */
    /*      area of interest.                                               */
    /* -------------------------------------------------------------------- */
    int nShapeCount;
    int *panHits = SHPTreeFindLikelyShapes(hTree, padfBoundsMin, padfBoundsMax,
                                           &nShapeCount);

    /* -------------------------------------------------------------------- */
    /*      Read all of these shapes, and establish whether the shape's     */
    /*      bounding box actually intersects the area of interest.  Note    */
    /*      that the bounding box could intersect the area of interest,     */
    /*      and the shape itself still not cross it but we don't try to     */
    /*      address that here.                                              */
    /* -------------------------------------------------------------------- */
    for (int i = 0; i < nShapeCount; i++)
    {
        SHPObject *psObject = SHPReadObject(hTree->hSHP, panHits[i]);
        if (psObject == NULL)
            continue;

        if (!SHPCheckBoundsOverlap(padfBoundsMin, padfBoundsMax,
                                   &(psObject->dfXMin), &(psObject->dfXMax),
                                   hTree->nDimension))
        {
            printf("Shape %d: not in area of interest, but fetched.\n",
                   panHits[i]);
        }
        else
        {
            printf("Shape %d: appears to be in area of interest.\n",
                   panHits[i]);
        }

        SHPDestroyObject(psObject);
    }

    if (nShapeCount == 0)
        printf("No shapes found in search.\n");
}
