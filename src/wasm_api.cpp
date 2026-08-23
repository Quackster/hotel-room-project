#include "Renderer.h"
#include "Room.h"
#include "Camera.h"
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <cstdio>

extern "C" {

EMSCRIPTEN_KEEPALIVE
void initialize(int width, int height) {
    getRenderer().init(width, height);
}

EMSCRIPTEN_KEEPALIVE
void resize(int width, int height) {
    getRenderer().resize(width, height);
}

EMSCRIPTEN_KEEPALIVE
void render() {
    getRenderer().render();
}

EMSCRIPTEN_KEEPALIVE
void selectTile(int x, int y) {
    getRenderer().selectTile(x,y);
}

EMSCRIPTEN_KEEPALIVE
void setCamera(float yaw, float pitch, float zoom) {
    auto& cam = getRenderer().getCamera();
    cam.yaw = yaw;
    cam.pitch = pitch;
    cam.setZoom(zoom);
}

EMSCRIPTEN_KEEPALIVE
void setCameraYawPitchZoom(float yaw, float pitch, float zoom) {
    setCamera(yaw,pitch,zoom);
}

EMSCRIPTEN_KEEPALIVE
int getSelectedX() { return getRoomState().selectedTile.x; }
EMSCRIPTEN_KEEPALIVE
int getSelectedY() { return getRoomState().selectedTile.y; }
EMSCRIPTEN_KEEPALIVE
int getRoomWidth() { return getRoomState().width; }
EMSCRIPTEN_KEEPALIVE
int getRoomHeight() { return getRoomState().height; }

EMSCRIPTEN_KEEPALIVE
void pickTile(float ndcX, float ndcY) {
    int tx, tz;
    auto& cam = getRenderer().getCamera();
    auto& room = getRoomState();
    if (cam.pickTile(ndcX, ndcY, tx, tz, room.width, room.height)) {
        selectTile(tx,tz);
    }
}

EMSCRIPTEN_KEEPALIVE
void handleClick(float canvasX, float canvasY, float canvasW, float canvasH) {
    float ndcX = (canvasX / canvasW) * 2.0f - 1.0f;
    float ndcY = 1.0f - (canvasY / canvasH) * 2.0f;
    pickTile(ndcX, ndcY);
}

} // extern C

// Optional main for standalone
int main() {
    // Emscripten will call this; we defer init to JS calling initialize()
    printf("WASM module loaded\n");
    return 0;
}
