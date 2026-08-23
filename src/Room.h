#pragma once

struct TileCoord {
    int x = 0;
    int y = 0;
    bool operator==(const TileCoord& o) const { return x==o.x && y==o.y; }
};

struct RoomState {
    int width = 8;
    int height = 6;
    TileCoord selectedTile{4, 3};
    float wallHeight = 2.2f;
    float wallThickness = 0.15f;
    // Door on back wall (Z = height) spanning X in [doorX0, doorX1]
    // Width exactly 1 tile to match floor grid; floor tiles are 1 unit.
    float doorX0 = 2.0f;
    float doorX1 = 3.0f;
    float doorHeight = 1.8f;
};

// Global room state accessor
RoomState& getRoomState();
void setSelectedTile(int x, int y);
bool isTileInside(int x, int y);
