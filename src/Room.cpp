#include "Room.h"

static RoomState g_room;

RoomState& getRoomState() { return g_room; }

void setSelectedTile(int x, int y) {
    if (x >= 0 && x < g_room.width && y >= 0 && y < g_room.height) {
        g_room.selectedTile.x = x;
        g_room.selectedTile.y = y;
    }
}

bool isTileInside(int x, int y) {
    return x >= 0 && x < g_room.width && y >= 0 && y < g_room.height;
}
