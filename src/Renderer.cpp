#include "Renderer.h"
#include <GLES2/gl2.h>
#include <cstdio>
#include <emscripten/html5.h>

static Renderer g_renderer;
Renderer& getRenderer() { return g_renderer; }

// Shader sources
static const char* VERT_SRC = R"(
attribute vec3 aPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";
static const char* FRAG_SRC = R"(
precision mediump float;
uniform vec3 uColor;
void main() {
    gl_FragColor = vec4(uColor, 1.0);
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok=0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok){
        char log[512]; glGetShaderInfoLog(s,512,nullptr,log);
        printf("shader compile error: %s\n", log);
    }
    return s;
}

bool Renderer::createShaderProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, VERT_SRC);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint ok=0; glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if(!ok){
        char log[512]; glGetProgramInfoLog(program,512,nullptr,log);
        printf("program link error: %s\n", log);
        return false;
    }
    glDeleteShader(vs); glDeleteShader(fs);
    aPosLoc = glGetAttribLocation(program, "aPos");
    uMVP = glGetUniformLocation(program, "uMVP");
    uColor = glGetUniformLocation(program, "uColor");
    return true;
}

static void setColor(GLint loc, const float c[3]) {
    glUniform3f(loc, c[0], c[1], c[2]);
}

void Renderer::buildGeometry() {
    destroyGeometry();
    RoomState& room = getRoomState();

    floorBase = createFloorBase(room.width, room.height);
    floorBase.upload();

    // inset = 1/64 = 0.015625 => with pixelsPerWorld = 32*k, gap = 2*inset*ppw = ppw/32 = k integer pixels, crisp
    // thickness like [Image 1] single tile — extruded block (programmatic)
    constexpr float kTileInset = 1.0f/64.0f;
    constexpr float kTileThickness = 0.125f; // 4/32 → 8px at ppw64, 4px at ppw32, integer crisp
    floorTiles = createFloorTiles(room.width, room.height, kTileInset);
    floorTiles.upload();
    floorTileSides = createFloorTilesWithSides(room.width, room.height, kTileInset, kTileThickness);
    floorTileSides.upload();

    // floor thickness skirt as single mesh split into 4? Use one mesh — match per-tile thickness
    Mesh skirt = createFloorThickness(room.width, room.height, 0.125f);
    // We'll repurpose floorSkirt[0] to hold it
    floorSkirt[0] = skirt;
    floorSkirt[0].upload();

    float W = (float)room.width;
    float H = (float)room.height;
    float t = room.wallThickness;
    float wh = room.wallHeight;
    float dh = room.doorHeight;
    float dx0 = room.doorX0;
    float dx1 = room.doorX1;
    // walls extend to bottom of floor (entirely solid), floor thickness 0.125
    float yb = -0.125f; // bottom Y flush with floor bottom

    // Back wall Z = H — inner face should face south (-Z) toward interior
    // so interior view (camera SW looking NE) sees lit inner walls.
    // Winding is reversed vs before (which faced +Z outer).
    {
        Mesh m;
        addQuad(m.vertices, m.indices,
            0,wh,H,
            dx0,wh,H,
            dx0,yb,H,
            0,yb,H);
        backWallLeft = m; backWallLeft.upload();
    }
    {
        Mesh m;
        addQuad(m.vertices, m.indices,
            dx1,wh,H,
            W,wh,H,
            W,yb,H,
            dx1,yb,H);
        backWallRight = m; backWallRight.upload();
    }
    {
        Mesh m;
        addQuad(m.vertices, m.indices,
            dx0,wh,H,
            dx1,wh,H,
            dx1,dh,H,
            dx0,dh,H);
        backWallTop = m; backWallTop.upload();
    }
    // Back wall thickness top faces (darker edge)
    {
        Mesh m;
        // top of wall left segment
        addQuad(m.vertices, m.indices,
            0,wh,H,
            0,wh,H+t,
            dx0,wh,H+t,
            dx0,wh,H);
        // top of right segment
        addQuad(m.vertices, m.indices,
            dx1,wh,H,
            dx1,wh,H+t,
            W,wh,H+t,
            W,wh,H);
        // top above door
        addQuad(m.vertices, m.indices,
            dx0,wh,H,
            dx0,wh,H+t,
            dx1,wh,H+t,
            dx1,wh,H);
        backWallEdgeLeft = m; backWallEdgeLeft.upload();
    }
    // Door jamb inner sides (reveal thickness) — make solid, correct winding for outer solid view
    {
        Mesh m;
        // left jamb (X = dx0, faces +X east toward doorway) — extend to bottom
        addQuad(m.vertices, m.indices,
            dx0,yb,H+t,
            dx0,yb,H,
            dx0,dh,H,
            dx0,dh,H+t);
        // right jamb (X = dx1, faces -X west toward doorway)
        addQuad(m.vertices, m.indices,
            dx1,yb,H,
            dx1,yb,H+t,
            dx1,dh,H+t,
            dx1,dh,H);
        // header underside (Y=dh, faces -Y down)
        addQuad(m.vertices, m.indices,
            dx0,dh,H,
            dx1,dh,H,
            dx1,dh,H+t,
            dx0,dh,H+t);
        // outer back faces — must face +Z north (solid outer)
        // Back outer face left (0..dx0)
        addQuad(m.vertices, m.indices,
            0,yb,H+t,
            dx0,yb,H+t,
            dx0,wh,H+t,
            0,wh,H+t);
        // Back outer face right (dx1..W)
        addQuad(m.vertices, m.indices,
            dx1,yb,H+t,
            W,yb,H+t,
            W,wh,H+t,
            dx1,wh,H+t);
        // Back outer above door (dx0..dx1, dh..wh)
        addQuad(m.vertices, m.indices,
            dx0,dh,H+t,
            dx1,dh,H+t,
            dx1,wh,H+t,
            dx0,wh,H+t);
        // Side caps for back wall thickness — make entirely solid outer
        // West cap at X=0, Z H..H+t, Y yb..wh, normal -X
        addQuad(m.vertices, m.indices,
            0,yb,H,
            0,yb,H+t,
            0,wh,H+t,
            0,wh,H);
        // East cap at X=W
        addQuad(m.vertices, m.indices,
            W,yb,H+t,
            W,yb,H,
            W,wh,H,
            W,wh,H+t);
        // bottom caps for back wall thickness (Y=yb, faces -Y down) — makes bottom solid
        addQuad(m.vertices, m.indices,
            0,yb,H+t,
            0,yb,H,
            dx0,yb,H,
            dx0,yb,H+t);
        addQuad(m.vertices, m.indices,
            dx1,yb,H+t,
            dx1,yb,H,
            W,yb,H,
            W,yb,H+t);
        addQuad(m.vertices, m.indices,
            dx0,yb,H+t,
            dx1,yb,H+t,
            dx1,yb,H,
            dx0,yb,H);
        // Caps at doorway edges for thickness side closure (at X=dx0 and dx1, top strip)
        // left top cap
        addQuad(m.vertices, m.indices,
            dx0,wh,H,
            dx0,wh,H+t,
            dx0,dh,H+t,
            dx0,dh,H);
        // right top cap
        addQuad(m.vertices, m.indices,
            dx1,wh,H+t,
            dx1,wh,H,
            dx1,dh,H,
            dx1,dh,H+t);
        backWallEdgeRight = m; backWallEdgeRight.upload();
    }

    // Right wall X = W, Z 0..H — extend to bottom yb
    {
        Mesh m;
        addQuad(m.vertices, m.indices,
            W,yb,0,
            W,yb,H,
            W,wh,H,
            W,wh,0);
        rightWall = m; rightWall.upload();
    }
    {
        // top
        Mesh m;
        addQuad(m.vertices, m.indices,
            W,wh,0,
            W,wh,H,
            W+t,wh,H,
            W+t,wh,0);
        rightWallTop = m; rightWallTop.upload();
    }
    {
        // outer side + thickness edge — entirely solid, extend to bottom
        Mesh m;
        addQuad(m.vertices, m.indices,
            W+t,yb,H,
            W+t,yb,0,
            W+t,wh,0,
            W+t,wh,H);
        // front edge Z=0 — faces -Z south (solid outer)
        addQuad(m.vertices, m.indices,
            W+t,yb,0,
            W,yb,0,
            W,wh,0,
            W+t,wh,0);
        // back edge Z=H — faces +Z north (will be interior to corner pillar, but keep for solid)
        addQuad(m.vertices, m.indices,
            W,yb,H,
            W+t,yb,H,
            W+t,wh,H,
            W,wh,H);
        // bottom caps for right wall (Y=yb, faces -Y)
        addQuad(m.vertices, m.indices,
            W,yb,0,
            W+t,yb,0,
            W+t,yb,H,
            W,yb,H);
        addQuad(m.vertices, m.indices,
            W,yb,H+t,
            W+t,yb,H+t,
            W+t,yb,H,
            W,yb,H);
        // corner pillar at W..W+t, H..H+t — makes outer join entirely solid
        // north face (+Z)
        addQuad(m.vertices, m.indices,
            W,yb,H+t,
            W+t,yb,H+t,
            W+t,wh,H+t,
            W,wh,H+t);
        // east face (+X)
        addQuad(m.vertices, m.indices,
            W+t,yb,H+t,
            W+t,yb,H,
            W+t,wh,H,
            W+t,wh,H+t);
        // top of pillar (+Y)
        addQuad(m.vertices, m.indices,
            W,wh,H,
            W,wh,H+t,
            W+t,wh,H+t,
            W+t,wh,H);
        // bottom of pillar (Y=yb)
        addQuad(m.vertices, m.indices,
            W,yb,H+t,
            W+t,yb,H+t,
            W+t,yb,H,
            W,yb,H);
        rightWallEdge = m; rightWallEdge.upload();
    }

    glGenBuffers(1, &selVbo);
}

void Renderer::destroyGeometry() {
    floorBase.destroy();
    floorTiles.destroy();
    floorTileSides.destroy();
    for(int i=0;i<4;i++) floorSkirt[i].destroy();
    backWallLeft.destroy(); backWallRight.destroy(); backWallTop.destroy();
    backWallEdgeLeft.destroy(); backWallEdgeRight.destroy();
    rightWall.destroy(); rightWallTop.destroy(); rightWallEdge.destroy();
    if(selVbo){ glDeleteBuffers(1,&selVbo); selVbo=0; }
}

static float snapOrthoFor32(int canvasH) {
    // Make tile (1 world unit) = N*32 pixels exactly → no blur
    // pixelsPerWorld = canvasH / orthoHeight  → ortho = canvasH / (32*k)
    // choose k so ortho ≈ 11 (Image 1 framing)
    const float target = 11.0f;
    int k = (int)roundf(canvasH / (32.0f * target));
    if (k < 1) k = 1;
    if (k > 8) k = 8;
    float snapped = canvasH / (32.0f * k);
    // keep within 9..13 to preserve composition
    if (snapped < 9) snapped = 9;
    if (snapped > 13) snapped = 13;
    return snapped;
}
static float orthoFor64x32(int canvasW, int canvasH) {
    // Initial view: tile (inset) must be exactly 64×32 pixels, POV-aware
    // Search ortho so projected diamond of a central tile is 64×32
    Camera tmp;
    tmp.yaw = 225.0f * 3.14159265f / 180.0f;
    tmp.pitch = 30.0f * 3.14159265f / 180.0f;
    tmp.centerX = 4.0f; tmp.centerZ = 3.0f;
    tmp.setAspect((float)canvasW, (float)canvasH);
    tmp.zoom = 1.0f;
    constexpr float kInset = 1.0f/64.0f;
    // binary search ortho 8..20 for 64x32
    float lo = 4.0f, hi = 60.0f, best = 11.0f;
    float bestErr = 1e9;
    for(int iter=0; iter<40; ++iter){
        float mid = (lo+hi)*0.5f;
        tmp.orthoBaseHeight = mid;
        float vp[16]; tmp.getViewProj(vp);
        float worlds[4][3] = {{4+kInset,0.02f,3+kInset},{5-kInset,0.02f,3+kInset},{5-kInset,0.02f,4-kInset},{4+kInset,0.02f,4-kInset}};
        float sx[4], sy[4];
        for(int i=0;i<4;++i){
            float x=worlds[i][0], y=worlds[i][1], z=worlds[i][2];
            float px=vp[0]*x+vp[4]*y+vp[8]*z+vp[12];
            float py=vp[1]*x+vp[5]*y+vp[9]*z+vp[13];
            float pw=vp[3]*x+vp[7]*y+vp[11]*z+vp[15];
            float ndcX=px/pw, ndcY=py/pw;
            sx[i]=(ndcX*0.5f+0.5f)*canvasW;
            sy[i]=(ndcY*0.5f+0.5f)*canvasH;
        }
        float minX=sx[0], maxX=sx[0], minY=sy[0], maxY=sy[0];
        for(int i=1;i<4;++i){ if(sx[i]<minX)minX=sx[i]; if(sx[i]>maxX)maxX=sx[i]; if(sy[i]<minY)minY=sy[i]; if(sy[i]>maxY)maxY=sy[i];}
        float w = maxX-minX, h = maxY-minY;
        float err = fabsf(w-64.0f)+fabsf(h-32.0f);
        if(err < bestErr){ bestErr=err; best=mid; }
        // w/h decreases as ortho increases (zoom out)
        if(w > 64.0f) lo = mid;
        else hi = mid;
    }
    return best;
}
bool Renderer::init(int width, int height) {
    canvasW = width; canvasH = height;
    camera.setAspect((float)width, (float)height);
    // adjust center to room
    RoomState& r = getRoomState();
    camera.centerX = r.width * 0.5f;
    camera.centerZ = r.height * 0.5f;
    // initial view: tiles exactly 64×32 pixels, POV-aware, crisp (programmatic)
    camera.orthoBaseHeight = orthoFor64x32(width, height);
    camera.zoom = 1.0f;

    // Create WebGL context if not already created (Emscripten)
    static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE s_ctx = 0;
    if (s_ctx <= 0) {
        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.alpha = false;
        attrs.depth = true;
        attrs.stencil = false;
        attrs.antialias = false; // never blurred — crisp pixels
        attrs.premultipliedAlpha = false;
        attrs.preserveDrawingBuffer = false;
        attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_DEFAULT;
        attrs.failIfMajorPerformanceCaveat = false;
        attrs.majorVersion = 2;
        attrs.minorVersion = 0;
        attrs.enableExtensionsByDefault = true;
        s_ctx = emscripten_webgl_create_context("#canvas", &attrs);
        if (s_ctx <= 0) {
            attrs.majorVersion = 1;
            s_ctx = emscripten_webgl_create_context("#canvas", &attrs);
            if (s_ctx <= 0) {
                printf("Failed to create WebGL context\n");
                return false;
            }
        }
        emscripten_webgl_make_context_current(s_ctx);
    } else {
        emscripten_webgl_make_context_current(s_ctx);
    }

    if(!createShaderProgram()) return false;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    buildGeometry();
    initialized = true;
    resize(width,height);
    return true;
}

void Renderer::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    canvasW = width; canvasH = height;
    camera.setAspect((float)width, (float)height);
    // Keep tiles 64×32 and divisible by 32 on resize — recompute ortho for current canvas
    camera.orthoBaseHeight = orthoFor64x32(width, height);
    // Only call glViewport if a WebGL context is current (avoid crash if
    // JS calls resize before initialize)
    if (emscripten_webgl_get_current_context() != 0) {
        glViewport(0,0,width,height);
    }
}

void Renderer::selectTile(int x, int y) {
    setSelectedTile(x,y);
}

void Renderer::render() {
    if(!initialized) return;
    glClearColor(palette.background[0], palette.background[1], palette.background[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);
    float vp[16]; camera.getViewProj(vp);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, vp);

    // Enable depth
    glEnable(GL_DEPTH_TEST);

    // Floor — disable cull to ensure +Y faces visible (and skirt sides)
    glDisable(GL_CULL_FACE);
    // Draw floor grid base (darker)
    setColor(uColor, palette.floorGrid);
    floorBase.draw(aPosLoc);

    // Draw floor tiles — top only (olive like [Image 1] tile top)
    setColor(uColor, palette.floor);
    floorTiles.draw(aPosLoc);

    // Draw per-tile extruded sides — darker olive like [Image 1] tile sides, programmatic
    setColor(uColor, palette.floorThickness);
    floorTileSides.draw(aPosLoc);

    // Draw floor skirt thickness (outer perimeter, also 32-aligned)
    // keep for outer border thickness continuity
    floorSkirt[0].draw(aPosLoc);
    glEnable(GL_CULL_FACE);

    // Draw walls — left/back wall darker (wallMain), right wall lighter (wallLight)
    // matches [Image 1] where right wall is pale bluish
    setColor(uColor, palette.wallMain);
    backWallLeft.draw(aPosLoc);
    backWallRight.draw(aPosLoc);
    backWallTop.draw(aPosLoc);
    setColor(uColor, palette.wallLight);
    rightWall.draw(aPosLoc);

    // Draw wall top edges darker
    setColor(uColor, palette.wallEdge);
    backWallEdgeLeft.draw(aPosLoc);
    rightWallTop.draw(aPosLoc);

    // Draw wall outer / jamb darker but slightly different; use edge color darker
    // For back outer we already have edge; draw with edge color but slightly lighter? Use wallEdge for consistency
    backWallEdgeRight.draw(aPosLoc);
    rightWallEdge.draw(aPosLoc);

    // Selection outline — same rotation axis as room (world-space), 64×32, pixelly
    {
        RoomState& room = getRoomState();
        constexpr float kInset = 1.0f/64.0f;
        float y = 0.02f; // slightly above floor to avoid z-fighting, same axis as tiles
        float x0 = room.selectedTile.x + kInset;
        float x1 = room.selectedTile.x + 1 - kInset;
        float z0 = room.selectedTile.y + kInset;
        float z1 = room.selectedTile.y + 1 - kInset;
        float verts[12] = {x0,y,z0, x1,y,z0, x1,y,z1, x0,y,z1};
        // Use same VP as room so highlight rotates exactly with room (same axis)
        // 64×32 already guaranteed by orthoFor64x32, 1/64 inset gives integer pixels, antialias=false → crisp
        glBindBuffer(GL_ARRAY_BUFFER, selVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(aPosLoc);
        glVertexAttribPointer(aPosLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glDisable(GL_CULL_FACE);
        // polygon offset not needed due to Y offset, but keep depth test
        setColor(uColor, palette.highlight);
        glLineWidth(1.0f);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
        glEnable(GL_CULL_FACE);
        glDisableVertexAttribArray(aPosLoc);
    }
    glUseProgram(0);
}
