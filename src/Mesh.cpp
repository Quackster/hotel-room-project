#include "Mesh.h"
#include "Room.h"

void Mesh::upload() {
    if (vertices.empty() || indices.empty()) return;
    if (vbo==0) glGenBuffers(1, &vbo);
    if (ibo==0) glGenBuffers(1, &ibo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);
    indexCount = (int)indices.size();
}
void Mesh::destroy() {
    if (vbo) { glDeleteBuffers(1,&vbo); vbo=0; }
    if (ibo) { glDeleteBuffers(1,&ibo); ibo=0; }
}
void Mesh::draw(GLint posAttrib) const {
    if (!vbo || !ibo || indexCount==0) return;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void*)0);
}

void addQuad(std::vector<float>& verts, std::vector<unsigned short>& idx,
             float x0,float y0,float z0,
             float x1,float y1,float z1,
             float x2,float y2,float z2,
             float x3,float y3,float z3) {
    unsigned short base = (unsigned short)(verts.size()/3);
    verts.insert(verts.end(), {x0,y0,z0, x1,y1,z1, x2,y2,z2, x3,y3,z3});
    idx.insert(idx.end(), {base, (unsigned short)(base+1), (unsigned short)(base+2), base, (unsigned short)(base+2), (unsigned short)(base+3)});
}

Mesh createFloorBase(int width, int height) {
    Mesh m;
    // Single quad covering entire floor at Y=0, slightly below tiles (Y=-0.005)
    // Winding +Y (up) so visible from elevated camera (eye has +Y)
    // Keep Y offset minimal to avoid z-fighting but integer aligned
    float y = -0.005f;
    addQuad(m.vertices, m.indices,
        0,y,0,
        0,y,(float)height,
        (float)width,y,(float)height,
        (float)width,y,0);
    return m;
}

Mesh createFloorTiles(int width, int height, float inset) {
    Mesh m;
    for(int z=0; z<height; ++z) for(int x=0; x<width; ++x) {
        float x0 = x + inset;
        float x1 = x + 1 - inset;
        float z0 = z + inset;
        float z1 = z + 1 - inset;
        float y = 0.0f;
        addQuad(m.vertices, m.indices,
            x0,y,z0,
            x0,y,z1,
            x1,y,z1,
            x1,y,z0);
    }
    return m;
}
Mesh createFloorTilesWithSides(int width, int height, float inset, float thickness) {
    Mesh m;
    for(int z=0; z<height; ++z) for(int x=0; x<width; ++x) {
        float x0 = x + inset;
        float x1 = x + 1 - inset;
        float z0 = z + inset;
        float z1 = z + 1 - inset;
        float y1 = 0.0f;
        float y0 = -thickness;
        // Sides — extruded block like [Image 1] single tile (side olive darker)
        // Front (south)
        addQuad(m.vertices, m.indices,
            x0,y1,z0, x1,y1,z0, x1,y0,z0, x0,y0,z0);
        // Back (north)
        addQuad(m.vertices, m.indices,
            x1,y1,z1, x0,y1,z1, x0,y0,z1, x1,y0,z1);
        // Left (west)
        addQuad(m.vertices, m.indices,
            x0,y1,z1, x0,y1,z0, x0,y0,z0, x0,y0,z1);
        // Right (east)
        addQuad(m.vertices, m.indices,
            x1,y1,z0, x1,y1,z1, x1,y0,z1, x1,y0,z0);
        // Bottom (covers underside, optional)
        addQuad(m.vertices, m.indices,
            x0,y0,z1, x0,y0,z0, x1,y0,z0, x1,y0,z1);
    }
    return m;
}

Mesh createFloorThickness(int width, int height, float thickness) {
    Mesh m;
    float y0 = -thickness;
    float y1 = 0.0f;
    // Create skirt around perimeter: 4 quads
    // Front (Z=0)
    addQuad(m.vertices, m.indices,
        0,y1,0, (float)width,y1,0, (float)width,y0,0, 0,y0,0);
    // Back (Z=height)
    addQuad(m.vertices, m.indices,
        (float)width,y1,(float)height, 0,y1,(float)height, 0,y0,(float)height, (float)width,y0,(float)height);
    // Left (X=0)
    addQuad(m.vertices, m.indices,
        0,y1,(float)height, 0,y1,0, 0,y0,0, 0,y0,(float)height);
    // Right (X=width)
    addQuad(m.vertices, m.indices,
        (float)width,y1,0, (float)width,y1,(float)height, (float)width,y0,(float)height, (float)width,y0,0);
    return m;
}

Mesh createBackWallWithDoor(const RoomState& room) {
    // For convenience we split into meshes externally; this function builds a combined mesh
    // but we will call specialized builders. Keep as empty.
    Mesh m;
    return m;
}

Mesh createRightWall(const RoomState& room) {
    Mesh m;
    float x = (float)room.width;
    float x2 = x + room.wallThickness;
    float y0 = 0, y1 = room.wallHeight;
    float z0 = 0, z1 = (float)room.height;
    // Outer face (facing interior, roughly -X direction visible? Actually facing -X?)
    // Right wall outer face at X = width (facing +X outermost? interior faces -X? Let's create both sides)
    // Main interior face (facing -X, towards room)
    addQuad(m.vertices, m.indices,
        x, y0, z0,
        x, y0, z1,
        x, y1, z1,
        x, y1, z0);
    // Top face
    // We'll handle top separately for color variation; but include here as separate mesh would be better.
    return m;
}

std::vector<float> createSelectionOutlineVertices(const TileCoord& tile) {
    float y = 0.02f;
    constexpr float kInset = 1.0f/64.0f; // match tile inset for pixel-perfect alignment
    float x0 = tile.x + kInset;
    float x1 = tile.x + 1 - kInset;
    float z0 = tile.y + kInset;
    float z1 = tile.y + 1 - kInset;
    // inset slightly to appear crisp inside tile, and also offset 0.015 to avoid covering grid exactly at edge?
    // Actually spec says outline follows four edges exactly. Use exact edges with tiny inset for visuals.
    // We'll use exact with small margin to avoid z-fighting; close to edges.
    // Use exact coordinates with 0.02 inset for crispness but keep visible.
    // Let's provide exact: x, x+1, z, z+1 with y offset.
    // We'll use inset 0.02 to stay inside tile visually.
    std::vector<float> v;
    v.reserve(12);
    // loop: (x0,z0)->(x1,z0)->(x1,z1)->(x0,z1)->close
    v.insert(v.end(), {x0,y,z0, x1,y,z0, x1,y,z1, x0,y,z1});
    return v;
}
