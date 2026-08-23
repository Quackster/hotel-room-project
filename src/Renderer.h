#pragma once
#include "Camera.h"
#include "Mesh.h"
#include "Room.h"

class Renderer {
public:
    bool init(int width, int height);
    void resize(int width, int height);
    void render();
    void selectTile(int x, int y);

    Camera& getCamera() { return camera; }

    // Colors (0..1)
    struct Palette {
        float background[3] = {0.0f, 0.0f, 0.0f};
        float floor[3]      = {0xA7/255.f, 0xA8/255.f, 0x75/255.f}; // #A7A875
        float floorGrid[3]  = {0x8F/255.f, 0x91/255.f, 0x66/255.f}; // #8F9166
        float floorThickness[3] = {0x7A/255.f, 0x7C/255.f, 0x5A/255.f};
        float wallMain[3]   = {0x99/255.f, 0x9B/255.f, 0xA6/255.f}; // #999BA6
        float wallLight[3]  = {0xAD/255.f, 0xB0/255.f, 0xBC/255.f}; // #ADB0BC
        float wallEdge[3]   = {0x66/255.f, 0x68/255.f, 0x71/255.f}; // #666871
        float highlight[3]  = {1.0f, 1.0f, 1.0f};
    } palette;

private:
    bool createShaderProgram();
    void buildGeometry();
    void destroyGeometry();

    GLuint program = 0;
    GLint aPosLoc = -1;
    GLint uMVP = -1;
    GLint uColor = -1;

    Camera camera;
    int canvasW = 800, canvasH = 600;
    bool initialized = false;

    // Geometry
    Mesh floorBase;
    Mesh floorTiles;
    Mesh floorTileSides;
    Mesh floorSkirt[4];
    Mesh backWallLeft, backWallRight, backWallTop, backWallEdgeLeft, backWallEdgeRight;
    Mesh rightWall, rightWallTop, rightWallEdge;
    // Selection outline as line loop (dynamic)
    GLuint selVbo = 0;
};

Renderer& getRenderer();
