#ifndef TILEMAP_H
#define TILEMAP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"

#define TILE_SIZE 32
#define MAX_MAP_COLS 200
#define MAX_MAP_ROWS 200

// Two separate grids: one purely for physics (collision layer),
// one purely for what gets drawn (decorations layer).
static int collisionData[MAX_MAP_ROWS][MAX_MAP_COLS];
static int decorationData[MAX_MAP_ROWS][MAX_MAP_COLS];
static int mapCols = 0;
static int mapRows = 0;

static void ResetMapGrids(void)
{
    for (int r = 0; r < MAX_MAP_ROWS; r++)
    for (int c = 0; c < MAX_MAP_COLS; c++)
    {
        collisionData[r][c] = -1;
        decorationData[r][c] = -1;
    }
    mapCols = 0;
    mapRows = 0;
}

// --- Direct .tmj (Tiled JSON) loading -------------------------------------
// Reads map data straight from your Tiled project file. No CSV export step,
// so this can never go out of sync with what you see in the Tiled editor.

static const char *FindBackward(const char *textStart, const char *from, const char *needle)
{
    size_t nlen = strlen(needle);
    const char *p = from;
    while (p >= textStart)
    {
        if (strncmp(p, needle, nlen) == 0) return p;
        p--;
    }
    return NULL;
}

// Parses one tile layer's "data":[...] array by locating it via the layer's
// "name" field, and fills the given grid. Tiled JSON uses 0 for empty tiles;
// we shift to -1 to match the convention used everywhere else in this file.
static bool LoadTMJLayer(const char *text, const char *layerName,
                          int grid[MAX_MAP_ROWS][MAX_MAP_COLS], int *outCols, int *outRows)
{
    char nameKey[128];
    snprintf(nameKey, sizeof(nameKey), "\"name\":\"%s\"", layerName);
    const char *namePos = strstr(text, nameKey);
    if (!namePos) { TraceLog(LOG_WARNING, "TMJ layer '%s' not found", layerName); return false; }

    const char *dataPos = FindBackward(text, namePos, "\"data\":[");
    const char *heightPos = FindBackward(text, namePos, "\"height\":");
    const char *widthPos = strstr(namePos, "\"width\":");
    if (!dataPos || !heightPos || !widthPos)
    {
        TraceLog(LOG_WARNING, "TMJ layer '%s' missing data/width/height", layerName);
        return false;
    }

    int width = (int)strtol(widthPos + strlen("\"width\":"), NULL, 10);
    int height = (int)strtol(heightPos + strlen("\"height\":"), NULL, 10);

    const char *ptr = dataPos + strlen("\"data\":[");
    int idx = 0;
    while (*ptr && *ptr != ']')
    {
        char *end;
        long val = strtol(ptr, &end, 10);
        if (end == ptr) { ptr++; continue; }

        int gid = (val == 0) ? -1 : (int)val; // Tiled JSON: 0 = empty, else use the gid as-is
        int row = idx / width;
        int col = idx % width;
        if (row < MAX_MAP_ROWS && col < MAX_MAP_COLS) grid[row][col] = gid;
        idx++;
        ptr = end;
        if (*ptr == ',') ptr++;
    }

    *outCols = width;
    *outRows = height;
    return true;
}

// Loads both layers directly from a .tmj file. layerNameCollision/Decoration
// should match the layer names in your Tiled project (default: "Collision",
// "Decorations").
static void LoadMapFromTMJ(const char *path, const char *layerNameCollision, const char *layerNameDecoration)
{
    ResetMapGrids();

    char *text = LoadFileText(path);
    if (!text) { TraceLog(LOG_WARNING, "Could not load %s", path); return; }

    int c1 = 0, r1 = 0, c2 = 0, r2 = 0;
    LoadTMJLayer(text, layerNameCollision, collisionData, &c1, &r1);
    LoadTMJLayer(text, layerNameDecoration, decorationData, &c2, &r2);

    mapCols = (c1 > c2) ? c1 : c2;
    mapRows = (r1 > r2) ? r1 : r2;

    UnloadFileText(text);
}

static bool IsTileSolid(int row, int col)
{
    if (row < 0 || row >= mapRows || col < 0 || col >= mapCols) return false;
    return collisionData[row][col] >= 0;
}

#define MAX_TILESETS 8

typedef struct
{
    int firstgid;       // Tiled's firstgid for this tileset (from the .tmx/.tsj)
    Texture2D texture;
    int columns;         // computed automatically from texture width
} TilesetInfo;

static TilesetInfo tilesets[MAX_TILESETS];
static int tilesetCount = 0;

// Register a tileset with the firstgid Tiled assigned it (see your .tmx file,
// e.g. <tileset firstgid="65" .../>). Order doesn't matter -- register all of
// them once at startup, right after loading their textures.
static void RegisterTileset(int firstgid, Texture2D texture)
{
    if (tilesetCount >= MAX_TILESETS) return;
    tilesets[tilesetCount].firstgid = firstgid;
    tilesets[tilesetCount].texture = texture;
    tilesets[tilesetCount].columns = texture.width / TILE_SIZE;
    tilesetCount++;
}

static void ClearTilesets(void)
{
    tilesetCount = 0;
}

// Given a raw tile id (gid), finds which registered tileset it
// belongs to: the tileset with the largest firstgid that is still <= gid.
static int FindTilesetIndex(int gid)
{
    int best = -1;
    for (int i = 0; i < tilesetCount; i++)
    {
        if (tilesets[i].firstgid <= gid &&
           (best == -1 || tilesets[i].firstgid > tilesets[best].firstgid))
            best = i;
    }
    return best;
}

// Draws every non-empty cell of one grid, routing each tile to its correct
// tileset texture via FindTilesetIndex.
static void DrawTileGrid(int grid[MAX_MAP_ROWS][MAX_MAP_COLS])
{
    for (int row = 0; row < mapRows; row++)
    for (int col = 0; col < mapCols; col++)
    {
        int gid = grid[row][col];
        if (gid < 0) continue;

        int idx = FindTilesetIndex(gid);
        if (idx < 0) continue; // no matching tileset registered, skip silently

        TilesetInfo *ts = &tilesets[idx];
        int localId = gid - ts->firstgid;

        Rectangle src = {
            (float)((localId % ts->columns) * TILE_SIZE),
            (float)((localId / ts->columns) * TILE_SIZE),
            (float)TILE_SIZE, (float)TILE_SIZE
        };
        Vector2 pos = { (float)(col * TILE_SIZE), (float)(row * TILE_SIZE) };
        DrawTextureRec(ts->texture, src, pos, WHITE);
    }
}

// Draws the full map: ground/collision tiles first (since that layer doubles
// as your visible ground), then decorations (trees) on top of it.
static void DrawMap(void)
{
    DrawTileGrid(collisionData);
    DrawTileGrid(decorationData);
}

// Generic AABB-vs-tilegrid collision, independent of your Player struct.
// Call after moving on the X axis.
static void ResolveTileCollisionX(float *x, float y, float w, float h, float *vx)
{
    int topRow    = (int)(y) / TILE_SIZE;
    int bottomRow = (int)(y + h - 1) / TILE_SIZE;
    int col = (*vx > 0) ? (int)(*x + w - 1) / TILE_SIZE
                        : (int)(*x) / TILE_SIZE;

    for (int row = topRow; row <= bottomRow; row++)
    {
        if (IsTileSolid(row, col))
        {
            if (*vx > 0) *x = col * TILE_SIZE - w;
            else         *x = (col + 1) * TILE_SIZE;
            *vx = 0;
            break;
        }
    }
}

// Call after moving on the Y axis. Returns true if the entity landed on top of a tile this call.
static bool ResolveTileCollisionY(float x, float *y, float w, float h, float *vy)
{
    int leftCol  = (int)(x) / TILE_SIZE;
    int rightCol = (int)(x + w - 1) / TILE_SIZE;
    int row = (*vy > 0) ? (int)(*y + h) / TILE_SIZE
                        : (int)(*y) / TILE_SIZE;

    for (int col = leftCol; col <= rightCol; col++)
    {
        if (IsTileSolid(row, col))
        {
            bool landed = (*vy > 0);
            if (landed) *y = row * TILE_SIZE - h;
            else        *y = (row + 1) * TILE_SIZE;
            *vy = 0;
            return landed;
        }
    }
    return false;
}

#endif // TILEMAP_H
