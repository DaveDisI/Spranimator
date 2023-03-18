#include "spranimator.cpp"

#include <windows.h>
#include <comdef.h>
#include <dsound.h>
#include <d2d1.h>
#include <dwrite.h>
#include <Shlobj.h>

#define winScaleToResolution(X, Y, W, H) X *= os->windowToResolutionRatio.x; \
                                         Y *= os->windowToResolutionRatio.y; \
                                         W *= os->windowToResolutionRatio.x; \
                                         H *= os->windowToResolutionRatio.y; 

struct Win32Interface {
    void (*appUpdate)(APPLICATION_TYPE*);

    HMODULE appDLLPtr;
    HWND windowHandle;
    
    ID2D1Factory* d2DFactory;
    IDWriteFactory* writeFactory;
    ID2D1SolidColorBrush* brush;
    ID2D1BitmapBrush* bitmapBrush;
    ID2D1Bitmap* brushBitmap;
    ID2D1HwndRenderTarget* renderTarget2D;
    IDWriteTextFormat* textFormat2D;
    f32 strokeSize;
    f32 fontSize;
    
    IDirectSound* directSound;
    LPDIRECTSOUNDBUFFER primaryAudioBuffer;
    LPDIRECTSOUNDBUFFER secondaryAudioBuffer;
    f32 (*audioUpdate)(f32 t);
    void (*audioUpdate2)(f32 t, f32* l, f32* r);
    f32 audioTime;
    u32 samplesPerSecond;
    u32 bytesPerSample;
    u32 sampleIndex;
    u32 latencySamples;
    u32 audioChannels;
    u32 audioWriteBufferSize;
};
static Win32Interface* win32Interface;
static APPLICATION_TYPE* win32Application;

static u32 winGetBytesPerPixelFromFormat(DXGI_FORMAT format){
    switch (format) {
        case DXGI_FORMAT_R8_UNORM: 
            return 1;
        case DXGI_FORMAT_BC4_SNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC1_UNORM: 
            return 2;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R32_FLOAT: 
        case DXGI_FORMAT_BC5_UNORM:
            return 4;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return 12;
        default: return 0;
    }
}

static void* winAllocateMemory(u32 amt){
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, amt);
}

static void winFreeMemory(void* m){
    HeapFree(GetProcessHeap(), 0, m);
}

static bool winSelectFileFromComputer(s8* fileBuffer){
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrFile = (LPSTR)fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFile[0] = '\0';
    ofn.nFilterIndex = 1;
    ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
    return GetOpenFileName(&ofn);
}

static bool winCheckIfDirectoryExists(s8* directoryName){
    DWORD attr = GetFileAttributes(directoryName);
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static bool winCheckIfFileExists(s8* fileName){
    DWORD attr = GetFileAttributes(fileName);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static bool winCreateDirectory(s8* fileName){
    return CreateDirectory(fileName, 0);
}

static bool winCreateFile(s8* fileName, void* data, u32 dataSize, bool append){
    HANDLE fh;
    DWORD type = append ? OPEN_EXISTING : CREATE_ALWAYS;
    fh = CreateFile(fileName, GENERIC_WRITE, 0, 0, type, FILE_ATTRIBUTE_NORMAL, 0);
    if(fh == INVALID_HANDLE_VALUE){
        return false;
    }
    
    DWORD fileSize = GetFileSize(fh, 0);
    SetFilePointer(fh, fileSize, 0, FILE_BEGIN);
    WriteFile(fh, data, dataSize, &fileSize, 0);
    CloseHandle(fh);
    return true;
}

static void winGetListOfFilesInDirectory(s8* dirName, s8* buffer, u32* strLen){
    WIN32_FIND_DATA fd;
    HANDLE fh = FindFirstFile(dirName, &fd);
    if (fh == INVALID_HANDLE_VALUE){
        return;
    }

    u32 bl = 0;
    do{
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
            s8* c = fd.cFileName;
            while(*c != '\0'){
                buffer[bl++] = *c;
                c++;
            }++
            buffer[bl++] = '\n';
            buffer[bl] = '\0';
        }
    } while(FindNextFile(fh, &fd));
    *strLen = bl;
}

static void winReadFileIntoBuffer(s8* fileName, void* buffer){
    HANDLE fh = CreateFile(fileName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(fh != INVALID_HANDLE_VALUE){
        DWORD fileSize = GetFileSize(fh, 0);
        BOOL b = ReadFile(fh, buffer, fileSize, 0, 0);
        if(!b){
            MessageBox(0, "", "", 0);
        }
        CloseHandle(fh);
    }
}

static void winBuildVariableString(s8* buffer, s8* text, ...){
    va_list argptr;
    va_start(argptr, text);
    vsprintf(buffer, text, argptr);
    va_end(argptr);
}

static void winBuildVariableStringV(s8* buffer, s8* text, va_list argptr){
    vsprintf(buffer, text, argptr);
}

static u64 winGetSystemTime(){
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

static FILETIME winGetFileWriteTime(s8* filename) {
    WIN32_FIND_DATA findData;
    HANDLE dllFileHandle = FindFirstFileA(filename, &findData);
    FindClose(dllFileHandle);
    return findData.ftLastWriteTime;
}

static void winReloadApplication(){
    if(win32Interface->appDLLPtr && !FreeLibrary(win32Interface->appDLLPtr)){
        return;
    }
    Sleep(200); // Needed otherwise CopyFile will fail with error 32: file in use by another process
    if (!CopyFile(APPLICATION_LIBRARY, "dll_file_copy.dll", false)) {
        MessageBox(0, "Could not copy dll to temp file", "ERROR", 0);
        return;
    }
    win32Interface->appDLLPtr = LoadLibrary("dll_file_copy.dll");
    if (win32Interface->appDLLPtr == 0) {
        MessageBox(0, "Could not load library", "ERROR", 0);
        return;
    }

    win32Interface->appUpdate = (void (__cdecl*)(APPLICATION_TYPE*))GetProcAddress(win32Interface->appDLLPtr, "updateApplication");
    if (*updateApplication == 0) {
        MessageBox(0, "Could not get address to updateGameState", "ERROR", 0);
        return;
    }
}

static void winSet2DViewport(u32 x, u32 y, u32 w, u32 h){
    D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Identity();
    trans._11 = (f32)w / os->windowSize.x;
    trans._22 = (f32)h / os->windowSize.y;
    trans._31 = x;
    trans._32 = y;
    win32Interface->renderTarget2D->SetTransform(trans);
}

static void winCreateSprite2D(Sprite2D* sprite, u32 width, u32 height, u32 format, void* data){
    D2D1_BITMAP_PROPERTIES bmprop = {};
    bmprop.pixelFormat.format = (DXGI_FORMAT)format;
    bmprop.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bmprop.dpiX = width;
    bmprop.dpiY = height;
    HRESULT hr = win32Interface->renderTarget2D->CreateBitmap(D2D1::SizeU(width, height), bmprop, (ID2D1Bitmap**)&sprite->bitmap);
    if(hr != S_OK){
        MessageBox(0, "Could not load bitmap", "Create Sprite 2D", 0);
        os->applicationRunning = false;
    }
    D2D1_RECT_U ru = D2D1::RectU(0, 0, width, height);

    u32 pitch = winGetBytesPerPixelFromFormat((DXGI_FORMAT)format) * width;
    ((ID2D1Bitmap*)sprite->bitmap)->CopyFromMemory(&ru, data, pitch);

    sprite->width = width;
    sprite->height = height;
    sprite->format = format;
    sprite->bounds = Vector4(0, 0, width, height);   

    D2D1_SIZE_F is = ((ID2D1Bitmap*)sprite->bitmap)->GetSize();
    D2D1_SIZE_U ps = ((ID2D1Bitmap*)sprite->bitmap)->GetPixelSize();

    sprite->pixelSize = Vector2(is.width / ps.width, is.height / ps.height);
}

static void winDeleteSprite2D(Sprite2D* sprite){
    if(sprite->bitmap){
        ((ID2D1Bitmap*)sprite->bitmap)->Release();
    }
}

static void winRenderSprite2D(Sprite2D* sprite, f32 x, f32 y, f32 w, f32 h){
    D2D1::Matrix3x2F rotation = D2D1::Matrix3x2F::Rotation((sprite->rotation * 360) / TAU, 
                                                 D2D1::Point2F(w * 0.5 + x, h * 0.5 + y));
    D2D1::Matrix3x2F tnsfm;
    win32Interface->renderTarget2D->GetTransform(&tnsfm);
    win32Interface->renderTarget2D->SetTransform(tnsfm * rotation);
    win32Interface->renderTarget2D->DrawBitmap((ID2D1Bitmap*)sprite->bitmap, 
                                                D2D1::RectF(x, y, x + w, y + h), 
                                                1, 
                                                D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                                                D2D1::RectF(sprite->bounds.x * sprite->pixelSize.x, 
                                                            sprite->bounds.y * sprite->pixelSize.y, 
                                                            sprite->bounds.x * sprite->pixelSize.x + sprite->bounds.z * sprite->pixelSize.x, 
                                                            sprite->bounds.y * sprite->pixelSize.y + sprite->bounds.w * sprite->pixelSize.y)); 
                                                            
    win32Interface->renderTarget2D->SetTransform(tnsfm);
}

static void winSetFontSize(f32 size){
    win32Interface->fontSize = size * os->windowToResolutionRatio.y;
}   

static void winSetColor2D(f32 r, f32 g, f32 b, f32 a){
    win32Interface->brush->SetColor(D2D1::ColorF(r, g, b, a));
}

static void winClearRenderTarget2D(f32 r, f32 g, f32 b, f32 a){
    win32Interface->renderTarget2D->Clear(D2D1::ColorF(r, g, b, a));
}

static void winDrawRectangle(f32 x, f32 y, f32 w, f32 h){
    winScaleToResolution(x, y, w, h);
    win32Interface->renderTarget2D->DrawRectangle(D2D1::RectF(x, y, x + w, y + h), win32Interface->brush, win32Interface->strokeSize);
}

static void winFillRectangle(f32 x, f32 y, f32 w, f32 h){
    winScaleToResolution(x, y, w, h);
    win32Interface->renderTarget2D->FillRectangle(D2D1::RectF(x, y, x + w, y + h), win32Interface->brush);
}

static void winDrawEllipse(f32 x, f32 y, f32 w, f32 h){
    winScaleToResolution(x, y, w, h);
    win32Interface->renderTarget2D->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), w, h), win32Interface->brush, win32Interface->strokeSize);
}

static void winFillEllipse(f32 x, f32 y, f32 w, f32 h){
    winScaleToResolution(x, y, w, h);
    win32Interface->renderTarget2D->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), w, h), win32Interface->brush);
}

static void winDrawLine(f32 x1, f32 y1, f32 x2, f32 y2){
    winScaleToResolution(x1, y1, x2, y2);
    win32Interface->renderTarget2D->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), win32Interface->brush, win32Interface->strokeSize);
}

static void winDrawText(s8* text, f32 x, f32 y){
    f32 w = os->windowResolution.x;
    f32 h = os->windowResolution.y;
    winScaleToResolution(x, y, w, h);
    IDWriteTextLayout* textLayout;
    u32 len = getLength(text);
    WCHAR str[256];
    mbstowcs(str, text, len + 1);
    win32Interface->writeFactory->CreateTextLayout(str, len, win32Interface->textFormat2D, w, h, &textLayout);
    textLayout->SetFontSize(win32Interface->fontSize, {0, len});
    win32Interface->renderTarget2D->DrawTextLayout(D2D1::Point2F(x, y), textLayout, win32Interface->brush, D2D1_DRAW_TEXT_OPTIONS_DISABLE_COLOR_BITMAP_SNAPPING);
    textLayout->Release();  
}

static void winGetTextDimensions(s8* text, f32* w, f32* h){
    IDWriteTextLayout* textLayout;
    u32 len = getLength(text);
    WCHAR str[256];
    mbstowcs(str, text, len + 1);
    win32Interface->writeFactory->CreateTextLayout(str, len, win32Interface->textFormat2D, os->windowSize.x, os->windowSize.y, &textLayout);
    textLayout->SetFontSize(win32Interface->fontSize, {0, len});
    DWRITE_TEXT_METRICS met;
    textLayout->GetMetrics(&met);
    *w = met.width;
    *h = met.height;
    textLayout->Release();  
}

static void winBegin2DRender(){
    win32Interface->renderTarget2D->BeginDraw();
}

static void winEnd2DRender(){
    win32Interface->renderTarget2D->EndDraw();
}

static void winSetStrokeSize(f32 size){
    win32Interface->strokeSize = size * os->windowToResolutionRatio.y;
}

static void winInitializeRendering2D(){
    win32Interface->strokeSize = 1;
    win32Interface->fontSize = 12;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED/*D2D1_FACTORY_TYPE_MULTI_THREADED*/, 
                                   &win32Interface->d2DFactory);
    RECT rc = {};
    if(!GetClientRect(win32Interface->windowHandle, &rc)){
        MessageBox(0, "Could not get client rect", "Init Render 2D", 0);
        os->applicationRunning = false;
        return;
    }
    D2D1_HWND_RENDER_TARGET_PROPERTIES hrtp = D2D1::HwndRenderTargetProperties(win32Interface->windowHandle, 
                                                                               D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top),
                                                                               D2D1_PRESENT_OPTIONS_IMMEDIATELY);
    if(!SUCCEEDED(win32Interface->d2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), hrtp, &win32Interface->renderTarget2D))){
        MessageBox(0, "Could not create render target", "Init Render 2D", 0);
        os->applicationRunning = false;
        return;
    }
    if(!SUCCEEDED(win32Interface->renderTarget2D->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &win32Interface->brush))){
        MessageBox(0, "Could not create brush", "Init Render 2D", 0);
        os->applicationRunning = false;
        return;
    }

    if(!SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown **)&win32Interface->writeFactory))){
        MessageBox(0, "Could not create Direct2D Factory", "Init Render 2D", 0);
        os->applicationRunning = false;
        return;
    }
    
    if(!SUCCEEDED(win32Interface->writeFactory->CreateTextFormat(L"arial", 0, 
                                                                 DWRITE_FONT_WEIGHT_NORMAL, 
                                                                 DWRITE_FONT_STYLE_NORMAL, 
                                                                 DWRITE_FONT_STRETCH_NORMAL, 
                                                                 win32Interface->fontSize, L"", 
                                                                 &win32Interface->textFormat2D))){
        MessageBox(0, "Could not create Text Format", "Init Render 2D", 0);
        os->applicationRunning = false;
        return; 
    }
}

static f32 winGetAudioStreamTime(){
    return win32Interface->audioTime;
}

static void winStartAudioStream(){
    win32Interface->secondaryAudioBuffer->Play(0, 0, DSBPLAY_LOOPING);
}

static void winStopAudioStream(){
    win32Interface->secondaryAudioBuffer->Stop();
}

static void winClearAudioBuffer(){
    VOID* region1;
    DWORD r1Size;
    VOID* region2;
    DWORD r2Size;
    if(SUCCEEDED(win32Interface->secondaryAudioBuffer->Lock(0, 0, &region1, &r1Size, &region2, &r2Size, DSBLOCK_ENTIREBUFFER))){
        switch(win32Interface->bytesPerSample){
            case 1:{
                u8* regionPtr = (u8*)region1;
                u32 totalSamples = (r1Size / win32Interface->audioChannels) / win32Interface->bytesPerSample;
                for(DWORD i = 0; i < totalSamples; i++){
                    for(u32 j = 0; j < win32Interface->audioChannels; j++){
                        *regionPtr++ = 0;
                    }
                }
                break;
            }
            case 2:{
                u16* regionPtr = (u16*)region1;
                u32 totalSamples = (r1Size / win32Interface->audioChannels) / win32Interface->bytesPerSample;
                for(DWORD i = 0; i < totalSamples; i++){
                    for(u32 j = 0; j < win32Interface->audioChannels; j++){
                        *regionPtr++ = 0;
                    }
                }
                break;
            }
            case 4:{
                u32* regionPtr = (u32*)region1;
                u32 totalSamples = (r1Size / win32Interface->audioChannels) / win32Interface->bytesPerSample;
                for(DWORD i = 0; i < totalSamples; i++){
                    for(u32 j = 0; j < win32Interface->audioChannels; j++){
                        *regionPtr++ = 0;
                    }
                }
                break;
            }
        }
        
        win32Interface->secondaryAudioBuffer->Unlock(region1, r1Size, region2, r2Size);
    }
}

static void winFillAudioBuffer(VOID* buffer, DWORD bufferSize){
    if(win32Interface->audioChannels == 1){
        switch(win32Interface->bytesPerSample){
            case 1:{
                s8* ptr = (s8*)buffer;
                DWORD sampleSize = bufferSize / win32Interface->audioChannels;
                for(DWORD i = 0; i < sampleSize; i++){
                    win32Interface->audioTime = (f32)win32Interface->sampleIndex / (f32)win32Interface->samplesPerSecond;
                    f32 v = win32Interface->audioUpdate(win32Interface->audioTime);
                    *ptr++ = ((v + 1) * 0.5) * 128;
                    
                    win32Interface->sampleIndex++;
                }
                break;
            }
            case 2:{
                s16* ptr = (s16*)buffer;
                DWORD sampleSize = (bufferSize / 2) / win32Interface->audioChannels;
                for(DWORD i = 0; i < sampleSize; i++){
                    win32Interface->audioTime = (f32)win32Interface->sampleIndex / (f32)win32Interface->samplesPerSecond;
                    f32 v = win32Interface->audioUpdate(win32Interface->audioTime);
                    *ptr++ = v * 32767;
                    
                    win32Interface->sampleIndex++;
                }
                break;
            }
            case 4:{
                s32* ptr = (s32*)buffer;
                DWORD sampleSize = (bufferSize / 4) / win32Interface->audioChannels;
                for(DWORD i = 0; i < sampleSize; i++){
                    win32Interface->audioTime = (f32)win32Interface->sampleIndex / (f32)win32Interface->samplesPerSecond;
                    f32 v = win32Interface->audioUpdate(win32Interface->audioTime);
                    *ptr++ = v * 2147483647;
                    
                    win32Interface->sampleIndex++;
                }
                break;
            }
        }
    }else if(win32Interface->audioChannels == 2){
        switch(win32Interface->bytesPerSample){
            case 1:{
                s8* ptr = (s8*)buffer;
                DWORD sampleSize = bufferSize / win32Interface->audioChannels;
                for(DWORD i = 0; i < sampleSize; i++){
                    win32Interface->audioTime = (f32)win32Interface->sampleIndex / (f32)win32Interface->samplesPerSecond;
                    f32 vl;
                    f32 vr; 
                    win32Interface->audioUpdate2(win32Interface->audioTime, &vl, &vr);
                    s16 vnl = vl * 127;
                    s16 vnr = vr * 127;
                    *ptr++ = vnl;
                    *ptr++ = vnr;
                    win32Interface->sampleIndex++;
                }
                break;
            }
            case 2:{
                s16* ptr = (s16*)buffer;
                DWORD sampleSize = (bufferSize / 2) / win32Interface->audioChannels;
                for(DWORD i = 0; i < sampleSize; i++){
                    win32Interface->audioTime = (f32)win32Interface->sampleIndex / (f32)win32Interface->samplesPerSecond;
                    f32 vl;
                    f32 vr; 
                    win32Interface->audioUpdate2(win32Interface->audioTime, &vl, &vr);
                    s16 vnl = vl * 32767;
                    s16 vnr = vr * 32767;
                    *ptr++ = vnl;
                    *ptr++ = vnr;
                    win32Interface->sampleIndex++;
                }
                break;
            }
            case 4:{
                s32* ptr = (s32*)buffer;
                DWORD sampleSize = (bufferSize / 4) / win32Interface->audioChannels;
                for(DWORD i = 0; i < sampleSize; i++){
                    win32Interface->audioTime = (f32)win32Interface->sampleIndex / (f32)win32Interface->samplesPerSecond;
                    f32 vl;
                    f32 vr; 
                    win32Interface->audioUpdate2(win32Interface->audioTime, &vl, &vr);
                    s16 vnl = vl * 2147483647;
                    s16 vnr = vr * 2147483647;
                    *ptr++ = vnl;
                    *ptr++ = vnr;
                    win32Interface->sampleIndex++;
                }
                break;
            }
        }
    }
}

static void winSetAudioStreamUpdateFunction1(f32 (*update)(f32)){
    win32Interface->audioUpdate = update;
}

static void winSetAudioStreamUpdateFunction2(void (*update)(f32, f32*, f32*)){
    win32Interface->audioUpdate2 = update;
}

static void winUninitializeAudioStream(){
    if(!SUCCEEDED(win32Interface->secondaryAudioBuffer->Stop())){
        MessageBox(0, "Could not stop playing buffer", "winUninitializeAudioStream", 0);
        os->applicationRunning = false;
        return;
    }

    win32Interface->secondaryAudioBuffer->Release();
    win32Interface->primaryAudioBuffer->Release();
    win32Interface->directSound->Release();
}

static void winInitializeAudioStream(u32 channels, u32 samplesPerSecond, u32 bytesPerSample, f32 bufferAdvanceInSeconds){
    win32Interface->samplesPerSecond = samplesPerSecond;
    win32Interface->bytesPerSample = bytesPerSample;
    win32Interface->latencySamples = win32Interface->samplesPerSecond / 100; 
    win32Interface->audioChannels = channels;

    if(!SUCCEEDED(CoInitializeEx(0, COINIT_MULTITHREADED))){
        MessageBox(0, "Could not run CoInitializeEx", "Init Audio Stream", 0);
        os->applicationRunning = false;
        return;
    }   
    if(!SUCCEEDED(DirectSoundCreate(0, (LPDIRECTSOUND*)&win32Interface->directSound, 0))){
        MessageBox(0, "Could not initialize DirectSound", "Init Audio Stream", 0);
        os->applicationRunning = false;
        return;
    }
    if(!SUCCEEDED(win32Interface->directSound->SetCooperativeLevel(win32Interface->windowHandle, DSSCL_PRIORITY))){
        MessageBox(0, "Could not set DirectSound Cooperation Level", "Init Audio Stream", 0);
        os->applicationRunning = false;
        return;
    }
    DSBUFFERDESC bufferDescription = {};
    bufferDescription.dwSize = sizeof(bufferDescription);
    bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
    if(!SUCCEEDED(win32Interface->directSound->CreateSoundBuffer(&bufferDescription, &win32Interface->primaryAudioBuffer, 0))){
        MessageBox(0, "Could not create Sound Buffer", "Init Audio Stream", 0);
        os->applicationRunning = false;
        return;
    }

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = channels;
    waveFormat.nSamplesPerSec = samplesPerSecond;
    waveFormat.wBitsPerSample = bytesPerSample * 8;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;
    if(!SUCCEEDED(win32Interface->primaryAudioBuffer->SetFormat(&waveFormat))){
        MessageBox(0, "Could not set Buffer Format", "Init Audio Stream", 0);
        os->applicationRunning = false;
        return;
    }

    bufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
    bufferDescription.dwBufferBytes = (win32Interface->samplesPerSecond * bytesPerSample * channels) * bufferAdvanceInSeconds;
    bufferDescription.lpwfxFormat = &waveFormat;
    if(!SUCCEEDED(win32Interface->directSound->CreateSoundBuffer(&bufferDescription, &win32Interface->secondaryAudioBuffer, 0))){
        MessageBox(0, "Could not create Sound Buffer", "Init Audio Stream", 0);
        os->applicationRunning = false;
        return;
    }

    win32Interface->audioWriteBufferSize = bufferDescription.dwBufferBytes;

    winClearAudioBuffer();
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_QUIT:
        case WM_CLOSE: {
            exit(0);
        }
        case WM_KEYDOWN:{
            os->keys[os->KEY_LEFT_SHIFT] = HIWORD(GetKeyState(VK_LSHIFT));
            os->keys[os->KEY_RIGHT_SHIFT] = HIWORD(GetKeyState(VK_RSHIFT));
            os->keys[wParam] = true;
            break;
        }
        case WM_KEYUP:{
            os->keys[os->KEY_LEFT_SHIFT] = HIWORD(GetKeyState(VK_LSHIFT));
            os->keys[os->KEY_RIGHT_SHIFT] = HIWORD(GetKeyState(VK_RSHIFT));
            os->keys[wParam] = false;
            break;
        }
        case WM_SYSKEYDOWN:{
            os->keys[wParam] = true;
            break;
        }
        case WM_SYSKEYUP:{
            os->keys[wParam] = false;
            break;
        }
        case WM_MOUSEMOVE:{
            POINT mousePosition;
            GetCursorPos(&mousePosition);
            ScreenToClient(win32Interface->windowHandle, &mousePosition);
            if (mousePosition.x != os->mousePosition.x || mousePosition.y != os->mousePosition.y) {
                os->mousePosition = Vector2((f32)mousePosition.x, mousePosition.y);
            }
            break;
        }
        case WM_LBUTTONDOWN :{
            os->mouseButtons[os->MOUSE_BUTTON_LEFT] = true;
            break;
        }
        case WM_LBUTTONUP :{
            os->mouseButtons[os->MOUSE_BUTTON_LEFT] = false;
            break;
        }
        case WM_MBUTTONDOWN :{
            os->mouseButtons[os->MOUSE_BUTTON_MIDDLE] = true;
            break;
        }
        case WM_MBUTTONUP :{
            os->mouseButtons[os->MOUSE_BUTTON_MIDDLE] = false;
            break;
        }
        case WM_RBUTTONDOWN :{
            os->mouseButtons[os->MOUSE_BUTTON_RIGHT] = true;
            break;
        }
        case WM_RBUTTONUP :{
            os->mouseButtons[os->MOUSE_BUTTON_RIGHT] = false;
            break;
        }
        case WM_CHAR: {
            os->lastTypedChar = wParam;
            os->charTypedFlag = true;
            break;
        }
        case WM_DISPLAYCHANGE:
        case WM_SIZE: {
            UINT nw = LOWORD(lParam);
            UINT nh = HIWORD(lParam);

            if(nw != os->windowSize.x || nh != os->windowSize.y){
                f32 xrat = os->windowResolution.x / (f32)nw;
                f32 yrat = os->windowResolution.y / (f32)nh;
                os->windowSize.x = nw;
                os->windowSize.y = nh;

                if(win32Interface->d2DFactory){
                    if((f32)nw > (f32)nh * os->aspectRatio){
                        f32 h = nh * yrat;
                        f32 w = nh * os->aspectRatio;
                        f32 x = nw / 2 - w / 2;  
                        winSet2DViewport(x * xrat, 0, w * xrat, h);
                    }else{
                        f32 w = nw * xrat;
                        f32 h = (f32)nw * (1.0f / os->aspectRatio);
                        f32 y = nh / 2 - h / 2;
                        winSet2DViewport(0, y * yrat, w, h * yrat);
                    }
                }
                os->windowToResolutionRatio.x = (f32)nw / os->windowResolution.x;
                os->windowToResolutionRatio.y = (f32)nh / os->windowResolution.y;
            }
            break;
        }
        case WM_MOUSEWHEEL: {
            os->mouseScrollDelta = (s16)HIWORD(wParam) / WHEEL_DELTA;
            break;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void winInitializeWindow(u32 width, u32 height, s8* title){
    os->windowSize.x = width;
    os->windowSize.y = height;
    os->windowResolution.x = width;
    os->windowResolution.y = height;
    os->aspectRatio = (f32)width / (f32)height;
    os->windowToResolutionRatio = Vector2(1);

    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSEX windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = title;
    RegisterClassEx(&windowClass);

    RECT windowRect = {0, 0, (LONG)width, (LONG)height};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    u32 monitorWidth = GetSystemMetrics(SM_CXSCREEN);
    u32 monitorHeight = GetSystemMetrics(SM_CYSCREEN);
    u32 windowX = monitorWidth / 2 - width / 2;
    u32 windowY = monitorHeight / 2 - height / 2;
    

    win32Interface->windowHandle = CreateWindow(windowClass.lpszClassName, title, WS_OVERLAPPEDWINDOW,
                                windowX, windowY, windowRect.right - windowRect.left,
                                windowRect.bottom - windowRect.top, 0, 0, hInstance, 0);

    if(!win32Interface->windowHandle){
        MessageBox(0, "Could not Create window", "Initialize Window", 0);
        os->applicationRunning = false;
        return;
    }

    #ifdef APP_FULLSCREEN
    ShowWindow(win32Interface->windowHandle, SW_MAXIMIZE);
    #else
    ShowWindow(win32Interface->windowHandle, SW_SHOW);
    #endif
}

static void winInitializeOS(){
    u32 totalSize = sizeof(Win32Interface) + sizeof(OSInterface) + sizeof(APPLICATION_TYPE);

    win32Interface = (Win32Interface*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, totalSize);
    if(!win32Interface){
        MessageBox(0, "Could not allocate application memory", "Initalize OS", 0);
        os->applicationRunning = false;
        return;
    }

    os = (OSInterface*)((u8*)win32Interface + sizeof(Win32Interface));
    win32Application = (APPLICATION_TYPE*)((u8*)os + sizeof(OSInterface));

    win32Application->os = os;

    GetModuleFileName(0, os->pathToApplication, MAX_PATH);
    s8* c = os->pathToApplication;
    while(*c){
        c++;
    }
    while(*c != '\\'){
        c--;
    }
    c++;
    *c = '\0';

    os->KEY_0 = 0x30;
    os->KEY_1 = 0x31;
    os->KEY_2 = 0x32;
    os->KEY_3 = 0x33;
    os->KEY_4 = 0x34;
    os->KEY_5 = 0x35;
    os->KEY_6 = 0x36;
    os->KEY_7 = 0x37;
    os->KEY_8 = 0x38;
    os->KEY_9 = 0x39;
    os->KEY_A = 0x41;
    os->KEY_B = 0x42;
    os->KEY_C = 0x43;
    os->KEY_D = 0x44;
    os->KEY_E = 0x45;
    os->KEY_F = 0x46;
    os->KEY_G = 0x47;
    os->KEY_H = 0x48;
    os->KEY_I = 0x49;
    os->KEY_J = 0x4A;
    os->KEY_K = 0x4B;
    os->KEY_L = 0x4C;
    os->KEY_M = 0x4D;
    os->KEY_N = 0x4E;
    os->KEY_O = 0x4F;
    os->KEY_P = 0x50;
    os->KEY_Q = 0x51;
    os->KEY_R = 0x52;
    os->KEY_S = 0x53;
    os->KEY_T = 0x54;
    os->KEY_U = 0x55;
    os->KEY_V = 0x56;
    os->KEY_W = 0x57;
    os->KEY_X = 0x58;
    os->KEY_Y = 0x59;
    os->KEY_Z = 0x5A;
    os->KEY_F1 = VK_F1;
    os->KEY_F2 = VK_F2;
    os->KEY_F3 = VK_F3;
    os->KEY_F4 = VK_F4;
    os->KEY_F5 = VK_F5;
    os->KEY_F6 = VK_F6;
    os->KEY_F7 = VK_F7;
    os->KEY_F8 = VK_F8;
    os->KEY_F9 = VK_F9;
    os->KEY_F10 = VK_F10;
    os->KEY_F11 = VK_F11;
    os->KEY_F12 = VK_F12;
    os->KEY_LEFT_SHIFT = VK_LSHIFT;
    os->KEY_RIGHT_SHIFT = VK_RSHIFT;
    os->KEY_LEFT_CTRL = 17;
    os->KEY_RIGHT_CTRL = VK_RCONTROL;
    os->KEY_UP = VK_UP;
    os->KEY_DOWN = VK_DOWN;
    os->KEY_LEFT = VK_LEFT;
    os->KEY_RIGHT = VK_RIGHT;
    os->KEY_SPACE = VK_SPACE;
    os->KEY_ENTER = VK_RETURN;
    os->KEY_ESCAPE = VK_ESCAPE;
    os->KEY_SEMI_COLON = VK_OEM_1;
    os->KEY_PAGE_UP = VK_PRIOR;
    os->KEY_PAGE_DOWN = VK_NEXT;
    os->KEY_TAB = VK_TAB;
    os->KEY_COMMA = VK_OEM_COMMA;
    os->KEY_PERIOD = VK_OEM_PERIOD;
    os->KEY_BK_SLASH = VK_OEM_5;
    os->KEY_FWD_SLASH = VK_OEM_2;
    os->KEY_BACKSPACE = VK_BACK;
    os->KEY_APOSTROPHE = VK_OEM_7;
    os->KEY_LEFT_BRACKET = VK_OEM_4;
    os->KEY_RIGHT_BRACKET = VK_OEM_6;
    os->KEY_EQUAL = VK_OEM_PLUS;
    os->KEY_ALT = VK_MENU;

    os->MOUSE_BUTTON_LEFT = 0;
    os->MOUSE_BUTTON_MIDDLE = 1;
    os->MOUSE_BUTTON_RIGHT = 2;
    os->GAMEPAD_A = 0;
    os->GAMEPAD_B = 1;
    os->GAMEPAD_X = 2;
    os->GAMEPAD_Y = 3;
    os->GAMEPAD_LB = 4;
    os->GAMEPAD_RB = 5;
    os->GAMEPAD_L3 = 6;
    os->GAMEPAD_R3 = 7;
    os->GAMEPAD_D_UP = 8;
    os->GAMEPAD_D_DOWN = 9;
    os->GAMEPAD_D_LEFT = 10;
    os->GAMEPAD_D_RIGHT = 11;
    os->GAMEPAD_START = 12;
    os->GAMEPAD_BACK = 13;

    os->BITMAP_FORMAT_BC1 = DXGI_FORMAT_BC1_UNORM;
    os->BITMAP_FORMAT_BC4S = DXGI_FORMAT_BC4_SNORM;
    os->BITMAP_FORMAT_BC4U = DXGI_FORMAT_BC4_UNORM;
    os->BITMAP_FORMAT_BC5 = DXGI_FORMAT_BC5_UNORM;
    os->BITMAP_FORMAT_R8_UNSIGNED = DXGI_FORMAT_R8_UNORM;
    os->BITMAP_FORMAT_RG8_UNSIGNED = DXGI_FORMAT_R8G8_UNORM;
    os->BITMAP_FORMAT_RGBA8_UNSIGNED = DXGI_FORMAT_R8G8B8A8_UNORM;

    os->initializeWindow = &winInitializeWindow;
    os->initializeRendering2D = &winInitializeRendering2D;
    os->initializeAudioStream = &winInitializeAudioStream;
    os->uninitializeAudioStream = &winUninitializeAudioStream;
    os->setAudioStreamUpdateFunction1 = &winSetAudioStreamUpdateFunction1;
    os->setAudioStreamUpdateFunction2 = &winSetAudioStreamUpdateFunction2;
    os->setColor2D = &winSetColor2D;
    os->clearRenderTarget2D = &winClearRenderTarget2D;
    os->drawRectangle = &winDrawRectangle;
    os->fillRectangle = &winFillRectangle;
    os->drawEllipse = &winDrawEllipse;
    os->fillEllipse = &winFillEllipse;
    os->drawLine = &winDrawLine;
    os->drawText = &winDrawText;
    os->begin2DRender = &winBegin2DRender;
    os->end2DRender = &winEnd2DRender;
    os->setStrokeSize = &winSetStrokeSize;
    os->setFontSize = &winSetFontSize;
    os->buildVariableString = &winBuildVariableString;
    os->buildVariableStringV = &winBuildVariableStringV;
    os->getAudioStreamTime = &winGetAudioStreamTime;
    os->getSystemTime = &winGetSystemTime;
    os->createSprite2D = &winCreateSprite2D;
    os->deleteSprite2D = &winDeleteSprite2D;
    os->renderSprite2D = &winRenderSprite2D;
    os->getTextDimensions = &winGetTextDimensions;
    os->checkIfDirectoryExists = &winCheckIfDirectoryExists;
    os->checkIfFileExists = &winCheckIfFileExists;
    os->createDirectory = &winCreateDirectory;
    os->createFile = &winCreateFile;
    os->getListOfFilesInDirectory = &winGetListOfFilesInDirectory;
    os->readFileIntoBuffer = &winReadFileIntoBuffer;
    os->allocateMemory = &winAllocateMemory;
    os->freeMemory = &winFreeMemory;
    os->startAudioStream = &winStartAudioStream;
    os->stopAudioStream = &winStopAudioStream;
    os->selectFileFromComputer = &winSelectFileFromComputer;

    os->applicationRunning = true;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    winInitializeOS();
    winInitializeWindow(APP_START_WIDTH, APP_START_HEIGHT, APP_TITLE);

    #ifdef APP_INIT_RENDER_2D
        winInitializeRendering2D();
    #endif

    initializeApplication(win32Application);
  
    FILETIME lastWriteTime = winGetFileWriteTime(APPLICATION_LIBRARY);
    winReloadApplication();

    LARGE_INTEGER startTime;
    LARGE_INTEGER endTime;
    LARGE_INTEGER perfomranceFreq;
    QueryPerformanceFrequency(&perfomranceFreq);
    QueryPerformanceCounter(&startTime);
    while (os->applicationRunning) {
        os->mouseScrollDelta = 0;
        MSG msg = {};
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // FILETIME currentTime = winGetFileWriteTime(APPLICATION_LIBRARY);                              
        // if (CompareFileTime(&currentTime, &lastWriteTime) != 0) {                          
        //     winReloadApplication(); 
        //     lastWriteTime = currentTime;                                                   
        // }

        win32Interface->appUpdate(win32Application);
        
        QueryPerformanceCounter(&endTime);
        os->deltaTime = (f32)(endTime.QuadPart - startTime.QuadPart) / (f32)perfomranceFreq.QuadPart;
        os->elapsedTime += os->deltaTime;
        startTime = endTime;
    }


    return 0;
}