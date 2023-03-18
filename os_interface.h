#pragma once
#include "mathematics.h"

struct Sprite2D {
    f32 rotation;
    u32 format;
    u32 width;
    u32 height;
    Vector4 bounds;
    Vector2 pixelSize;
    void* bitmap;
};

struct OSInterface {
    s8 windowTitle[128];
    s8 pathToApplication[512];
    bool keys[256];
    bool keyLocks[256];
    bool mouseButtons[8];
    bool mouseLocks[8];
    bool charTypedFlag;

    s8 lastTypedChar;

    Vector2 mousePosition;
    Vector2 windowSize;
    Vector2 windowResolution;
    Vector2 windowToResolutionRatio;

    s32 mouseScrollDelta;

    f32 deltaTime;
    f32 elapsedTime;
    f32 aspectRatio;

    bool applicationRunning;    

    void (*initializeWindow)(u32 width, u32 height, s8* title);
    void (*initializeRendering2D)();
    void (*initializeAudioStream)(u32 channels, u32 samplesPerSecond, u32 bytesPerSample, f32 bufferAdvanceInSeconds);
    void (*uninitializeAudioStream)();
    void (*setAudioStreamUpdateFunction1)(f32 (*func)(f32));
    void (*setAudioStreamUpdateFunction2)(void (*func)(f32, f32*, f32*));
    void (*setColor2D)(f32 r, f32 g, f32 b, f32 a);
    void (*clearRenderTarget2D)(f32 r, f32 g, f32 b, f32 a);
    void (*drawRectangle)(f32 x, f32 y, f32 w, f32 h);
    void (*fillRectangle)(f32 x, f32 y, f32 w, f32 h);
    void (*drawEllipse)(f32 x, f32 y, f32 w, f32 h);
    void (*fillEllipse)(f32 x, f32 y, f32 w, f32 h);
    void (*drawLine)(f32 x1, f32 y1, f32 x2, f32 y2);
    void (*drawText)(s8* text, f32 x, f32 y);
    void (*begin2DRender)();
    void (*end2DRender)();
    void (*setStrokeSize)(f32 size);
    void (*setFontSize)(f32 size);
    void (*buildVariableString)(s8* buffer, s8* text, ...);
    void (*buildVariableStringV)(s8* buffer, s8* text, va_list args);
    void (*createSprite2D)(Sprite2D* sprite, u32 width, u32 height, u32 format, void* data);
    void (*deleteSprite2D)(Sprite2D* sprite);
    void (*renderSprite2D)(Sprite2D* sprite, f32 x, f32 y, f32 w, f32 h);
    void (*getTextDimensions)(s8* text, f32* w, f32* h);
    void (*readFileIntoBuffer)(s8* file, void* buffer);
    void (*getListOfFilesInDirectory)(s8* dirName, s8* buffer, u32* strLen);
    void (*freeMemory)(void* m);
    void (*startAudioStream)();
    void (*stopAudioStream)();

    f32 (*getAudioStreamTime)();
    u64 (*getSystemTime)();
    bool (*checkIfDirectoryExists)(s8* dir);
    bool (*checkIfFileExists)(s8* file);
    bool (*createDirectory)(s8* dir);
    bool (*createFile)(s8* fileName, void* data, u32 dataSize, bool append);
    bool (*selectFileFromComputer)(s8* fileBuffer);
    void* (*allocateMemory)(u32 amt);



    u16 KEY_0;
    u16 KEY_1;
    u16 KEY_2;
    u16 KEY_3;
    u16 KEY_4;
    u16 KEY_5;
    u16 KEY_6;
    u16 KEY_7;
    u16 KEY_8;
    u16 KEY_9;
    u16 KEY_A;
    u16 KEY_B;
    u16 KEY_C;
    u16 KEY_D;
    u16 KEY_E;
    u16 KEY_F;
    u16 KEY_G;
    u16 KEY_H;
    u16 KEY_I;
    u16 KEY_J;
    u16 KEY_K;
    u16 KEY_L;
    u16 KEY_M;
    u16 KEY_N;
    u16 KEY_O;
    u16 KEY_P;
    u16 KEY_Q;
    u16 KEY_R;
    u16 KEY_S;
    u16 KEY_T;
    u16 KEY_U;
    u16 KEY_V;
    u16 KEY_W;
    u16 KEY_X;
    u16 KEY_Y;
    u16 KEY_Z;
    u16 KEY_F1;
    u16 KEY_F2;
    u16 KEY_F3;
    u16 KEY_F4;
    u16 KEY_F5;
    u16 KEY_F6;
    u16 KEY_F7;
    u16 KEY_F8;
    u16 KEY_F9;
    u16 KEY_F10;
    u16 KEY_F11;
    u16 KEY_F12;
    u16 KEY_LEFT_SHIFT;
    u16 KEY_RIGHT_SHIFT;
    u16 KEY_LEFT_CTRL;
    u16 KEY_RIGHT_CTRL;
    u16 KEY_UP;
    u16 KEY_DOWN;
    u16 KEY_LEFT;
    u16 KEY_RIGHT;
    u16 KEY_SPACE;
    u16 KEY_ENTER;
    u16 KEY_ESCAPE;
    u16 KEY_SEMI_COLON;
    u16 KEY_BACKSPACE;
    u16 KEY_PAGE_UP;
    u16 KEY_PAGE_DOWN;
    u16 KEY_TAB;
    u16 KEY_COMMA;
    u16 KEY_PERIOD;
    u16 KEY_BK_SLASH;
    u16 KEY_FWD_SLASH;
    u16 KEY_APOSTROPHE;
    u16 KEY_LEFT_BRACKET;
    u16 KEY_RIGHT_BRACKET;
    u16 KEY_EQUAL;
    u16 KEY_ALT;

    u8 MOUSE_BUTTON_LEFT;
    u8 MOUSE_BUTTON_MIDDLE;
    u8 MOUSE_BUTTON_RIGHT;

    u8 GAMEPAD_A;
    u8 GAMEPAD_B;
    u8 GAMEPAD_X;
    u8 GAMEPAD_Y;
    u8 GAMEPAD_LB;
    u8 GAMEPAD_RB;
    u8 GAMEPAD_L3;
    u8 GAMEPAD_R3;
    u8 GAMEPAD_D_UP;
    u8 GAMEPAD_D_DOWN;
    u8 GAMEPAD_D_LEFT;
    u8 GAMEPAD_D_RIGHT;
    u8 GAMEPAD_START;
    u8 GAMEPAD_BACK;

    u32 BITMAP_FORMAT_BC1;
    u32 BITMAP_FORMAT_BC4S;
    u32 BITMAP_FORMAT_BC4U;
    u32 BITMAP_FORMAT_BC5;
    u32 BITMAP_FORMAT_R8_UNSIGNED;
    u32 BITMAP_FORMAT_RG8_UNSIGNED;
    u32 BITMAP_FORMAT_RGBA8_UNSIGNED;
};

static OSInterface* os;