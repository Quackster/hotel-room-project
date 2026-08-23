// Native entry point — only for non-Emscripten builds; WASM entry is in wasm_api.cpp
#ifndef __EMSCRIPTEN__
int main() { return 0; }
#endif
