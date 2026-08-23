#pragma once
#include <vector>
#include <GLES2/gl2.h>

struct Mesh {
    std::vector<float> vertices; // xyz interleaved
    std::vector<unsigned short> indices;
    GLuint vbo = 0;
    GLuint ibo = 0;
    int indexCount = 0;

    void upload();
    void destroy();
    void draw(GLint posAttrib) const;
};

// Procedural geometry builders
Mesh createFloorBase(int width, int height);
Mesh createFloorTiles(int width, int height, float inset);
Mesh createFloorTilesWithSides(int width, int height, float inset, float thickness);
Mesh createFloorThickness(int width, int height, float thickness);
Mesh createBackWallWithDoor(const struct RoomState& room);
Mesh createRightWall(const struct RoomState& room);
Mesh createWallTop(const struct RoomState& room); // top edges darker
std::vector<float> createSelectionOutlineVertices(const struct TileCoord& tile);

// Helpers
void addQuad(std::vector<float>& verts, std::vector<unsigned short>& idx,
             float x0,float y0,float z0,
             float x1,float y1,float z1,
             float x2,float y2,float z2,
             float x3,float y3,float z3);
