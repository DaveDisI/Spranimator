#pragma once

#include "gui.h"
#include "png_extractor.h"

#define APP_START_WIDTH 1600
#define APP_START_HEIGHT 900
#define APP_INIT_RENDER_2D
#define APP_TITLE "Spranimator"
#define APPLICATION_TYPE Spranimator
#define APPLICATION_LIBRARY "lib_spranimator.dll"

struct Animation {
    Vector4 frames[1024];
    u32 currentFrame;
    u32 totalFrames;
    f32 delay;
    f32 frameTime;
};

struct Spranimator {
    OSInterface* os;
    GUI gui;

    Animation animations[1024];
    Animation* currentAnimation;
    u32 currentAnimationIndex;
    u32 totalAnimations;

    s8 textBuffer[512];
    u8 fileBuffer[MEGABYTE(512)];
    u8 pixelBuffer[MEGABYTE(512)];
    Sprite2D image;
    u32 bmWidth;
    u32 bmHeight;
    u32 componentsPerPixel;
    u32 bitsPerComponent;

    Vector2 subspriteStartPos;
    Vector2 subspriteEndPos;
    Vector2 displayPos;
    Vector2 displaySz;
    Vector2 offset;


    f32 mapZoom;
    f32 animationZoom;

    bool playAnimation;
    bool exportAnimation;
};

static Spranimator* s;

#define PH(X) X * os->windowResolution.x
#define PV(Y) Y * os->windowResolution.y

static void runControllPanel(f32 x, f32 y, f32 w, f32 h){

    if(button("Import", x + w * 0.5, y + h * 0.3, w * 0.2, h * 0.1)){
        if(os->selectFileFromComputer(s->textBuffer)){
            if(s->image.bitmap){
                os->deleteSprite2D(&s->image);
            }

            os->readFileIntoBuffer(s->textBuffer, s->fileBuffer);
            u8* fptr = s->fileBuffer;

            s->totalAnimations = RI_PTR(fptr, u8);
            for(u32 i = 0; i < s->totalAnimations; i++){
                Animation* an = &s->animations[i];
                an->totalFrames = RI_PTR(fptr, u8);
                an->delay = RI_PTR(fptr, f32);
                for(u32 j = 0; j < an->totalFrames; j++){
                    an->frames[j] = RI_PTR(fptr, Vector4); 
                }
            }

            s->bmWidth = RI_PTR(fptr, u16);
            s->bmHeight = RI_PTR(fptr, u16);
            u32 sz = s->bmWidth * s->bmHeight * 4;
            copyMemory(s->pixelBuffer, fptr, sz);
            s->componentsPerPixel = 4;
            s->bitsPerComponent = 8;
            os->createSprite2D(&s->image, s->bmWidth , s->bmHeight, os->BITMAP_FORMAT_RGBA8_UNSIGNED, s->pixelBuffer);

        }
    }

    if(s->exportAnimation){
        textInput(s->textBuffer, x + w * 0.2, y + h * 0.5, w * 0.4, h * 0.1);

        if(button("OK", x + w * 0.2, y + h * 0.7, w * 0.1, h * 0.1)){
            os->buildVariableString((s8*)s->fileBuffer, "C:/Users/Dave/Desktop/%s%s", s->textBuffer, ".anipack");

            u8* fptr = s->fileBuffer + getLength((s8*)s->fileBuffer) + 1;
            u8* dataStart = fptr;

            WI_PTR(fptr, s->totalAnimations, u8);

            for(u32 i = 0; i < s->totalAnimations; i++){
                Animation* an = &s->animations[i];
                WI_PTR(fptr, an->totalFrames, u8);
                WI_PTR(fptr, an->delay, f32);
                for(u32 j = 0; j < an->totalFrames; j++){
                    WI_PTR(fptr, an->frames[j], Vector4);
                }
            }

            WI_PTR(fptr, s->image.width, u16);
            WI_PTR(fptr, s->image.height, u16);
            u32 sz = s->image.width * s->image.height * 4;
            copyMemory(fptr, s->pixelBuffer, sz);
            fptr += sz;

            os->createFile((s8*)s->fileBuffer, dataStart, fptr - dataStart, false);

            s->exportAnimation = false;
        }

        if(button("Cancel", x + w * 0.4, y + h * 0.7, w * 0.1, h * 0.1)){
            s->exportAnimation = false;
        }

        return;
    }


    if(button("Load Image", x + w * 0.83, y + h * 0.3, w * 0.15, h * 0.1)){
        if(!os->selectFileFromComputer(s->textBuffer)){
            return;
        }

        f32* pix = uncompressPNG(s->textBuffer, &s->bmWidth, &s->bmHeight, &s->componentsPerPixel, &s->bitsPerComponent);

        if((s->componentsPerPixel == 3 || s->componentsPerPixel == 4) && s->bitsPerComponent == 8){
            u32 totalPixels = s->bmWidth * s->bmHeight * 4;
            for(u32 i = 0; i < totalPixels; i++){
                s->pixelBuffer[i] = pix[i] * 255;
            }

            if(s->image.bitmap){
                os->deleteSprite2D(&s->image);
            }

            os->createSprite2D(&s->image, s->bmWidth, s->bmHeight, os->BITMAP_FORMAT_RGBA8_UNSIGNED, s->pixelBuffer);
            s->totalAnimations = 1;
        }
        os->freeMemory(pix);
    }

    if(s->image.bitmap){
        if(button("Update Image", x + w * 0.83, y + h * 0.18, w * 0.15, h * 0.1)){
            if(os->selectFileFromComputer(s->textBuffer)){
                f32* pix = uncompressPNG(s->textBuffer, &s->bmWidth, &s->bmHeight, &s->componentsPerPixel, &s->bitsPerComponent);

                if((s->componentsPerPixel == 3 || s->componentsPerPixel == 4) && s->bitsPerComponent == 8){
                    u32 totalPixels = s->bmWidth * s->bmHeight * 4;
                    for(u32 i = 0; i < totalPixels; i++){
                        s->pixelBuffer[i] = pix[i] * 255;
                    }

                    if(s->image.bitmap){
                        os->deleteSprite2D(&s->image);
                    }

                    os->createSprite2D(&s->image, s->bmWidth, s->bmHeight, os->BITMAP_FORMAT_RGBA8_UNSIGNED, s->pixelBuffer);
                }
                os->freeMemory(pix);
            }
        }

        if(button("Save as Frame", x + w * 0.8, y + h * 0.45, w * 0.2, h * 0.1)){
            if(s->image.bitmap){
                Animation* a = s->currentAnimation;
                a->frames[a->totalFrames++] = Vector4(s->subspriteStartPos.x, s->subspriteStartPos.y, s->displaySz.x, s->displaySz.y);
            }
        }

        if(button("New Animation", x + w * 0.8, y + h * 0.7, w * 0.2, h * 0.1)){
            s->currentAnimationIndex = s->totalAnimations;
            s->totalAnimations++;
            s->currentAnimation = &s->animations[s->currentAnimationIndex];
        }

        if(button("Delete Animation", x + w * 0.15, y + h * 0.7, w * 0.2, h * 0.1)){
            if(s->totalAnimations > 0){
                for(u32 i = s->currentAnimationIndex; i < s->totalAnimations - 1; i++){
                    s->animations[i] = s->animations[i + 1];
                }
                s->totalAnimations--;

                s->animations[s->totalAnimations].currentFrame = 0;
                s->animations[s->totalAnimations].totalFrames = 0;

                s->currentAnimationIndex = 0;
                s->currentAnimation = &s->animations[s->currentAnimationIndex];
            }
        }

        if(button("<", x + w * 0.8, y + h * 0.57, w * 0.1, h * 0.1)){
            if(s->currentAnimationIndex > 0){
                s->currentAnimationIndex--;
                s->currentAnimation = &s->animations[s->currentAnimationIndex];
            }
        }
        if(button(">", x + w * 0.9, y + h * 0.57, w * 0.1, h * 0.1)){
            if(s->currentAnimationIndex < s->totalAnimations - 1){
                s->currentAnimationIndex++;
                s->currentAnimation = &s->animations[s->currentAnimationIndex];
            }
        }

        if(s->totalAnimations > 0){
            os->setColor2D(0, 0, 0, 1);
            os->setFontSize(h * 0.05);
            os->buildVariableString(s->textBuffer, "Animation %i of %i", s->currentAnimationIndex + 1, s->totalAnimations);
            os->drawText(s->textBuffer, x + w * 0.4, y + h * 0.7);

            if(button("Export Animations", x + w * 0.5, y + h * 0.5, w * 0.2, h * 0.1)){
                s->textBuffer[0] = '\0';
                s->exportAnimation = true;
                return;
            }
        }
    }

    Animation* a = s->currentAnimation;
    if(a->totalFrames > 0){
        checkBox(&s->playAnimation, x + w * 0.1, y + h * 0.90, w * 0.05, h * 0.08);
        os->setColor2D(0, 0, 0, 1);
        os->setFontSize(h * 0.05);
        os->drawText("Play", x + w * 0.01, y + h * 0.9);

        os->drawText("Delay", x + w * 0.72, y + h * 0.9);

        f32Input(&s->currentAnimation->delay, x + w * 0.6, y + h * 0.90, w * 0.1, h * 0.08);

        if(button("Delete Frame", x + w * 0.85, y + h * 0.90, w * 0.1, h * 0.08)){
            if(a->totalFrames > 0){
                for(u32 i = a->currentFrame; i < a->totalFrames - 1; i++){
                    a->frames[i] = a->frames[i + 1];
                }
                a->totalFrames--;

                if(a->currentFrame > 0){
                    a->currentFrame--;
                }
            }
        }

        if(button("Update Frame", x + w * 0.6, y + h * 0.8, w * 0.1, h * 0.08)){
            a->frames[a->currentFrame] = Vector4(s->subspriteStartPos.x, s->subspriteStartPos.y, s->displaySz.x, s->displaySz.y);
        }

        if(!s->playAnimation){
            if(button("<", x + w * 0.2, y + h * 0.90, w * 0.05, h * 0.08)){
                if(a->currentFrame > 0){
                    a->currentFrame--;
                }else{
                    a->currentFrame = a->totalFrames - 1;
                }
                
            }
            if(button(">", x + w * 0.3, y + h * 0.90, w * 0.05, h * 0.08)){
                if(a->currentFrame < a->totalFrames - 1){
                    a->currentFrame++;
                }else{
                    a->currentFrame = 0;
                }
            }

            

            os->setColor2D(0, 0, 0, 1);
            os->setFontSize(h * 0.05);
            os->buildVariableString(s->textBuffer, "Frame %i of %i", a->currentFrame + 1, a->totalFrames);
            os->drawText(s->textBuffer, x + w * 0.4, y + h * 0.9);
        }
    }


    if(button("<", x + w * 0.1, y + h * 0.1, w * 0.03, h * 0.05)){
        if(s->subspriteStartPos.x > 0){
            s->subspriteStartPos.x -= 1;
        }
    }

    if(button(">", x + w * 0.3, y + h * 0.1, w * 0.03, h * 0.05)){
        if(s->subspriteStartPos.x < s->image.width){
            s->subspriteStartPos.x += 1;
        }
    }

    if(button("<", x + w * 0.1, y + h * 0.2, w * 0.03, h * 0.05)){
        if(s->subspriteStartPos.y > 0){
            s->subspriteStartPos.y -= 1;
        }
    }

    if(button(">", x + w * 0.3, y + h * 0.2, w * 0.03, h * 0.05)){
        if(s->subspriteStartPos.y < s->image.height){
            s->subspriteStartPos.y += 1;
        }
    }

    if(button("<", x + w * 0.1, y + h * 0.3, w * 0.03, h * 0.05)){
        if(s->subspriteEndPos.x > s->subspriteStartPos.x){
            s->subspriteEndPos.x -= 1;
        }
    }

    if(button(">", x + w * 0.3, y + h * 0.3, w * 0.03, h * 0.05)){
        if(s->subspriteEndPos.x < s->image.width){
            s->subspriteEndPos.x += 1;
        }
    }

    if(button("<", x + w * 0.1, y + h * 0.4, w * 0.03, h * 0.05)){
        if(s->subspriteEndPos.y > s->subspriteStartPos.y){
            s->subspriteEndPos.y -= 1;
        }
    }

    if(button(">", x + w * 0.3, y + h * 0.4, w * 0.03, h * 0.05)){
        if(s->subspriteEndPos.y < s->image.height){
            s->subspriteEndPos.y += 1;
        }
    }
    

    os->setColor2D(0, 0, 0, 1);
    os->setStrokeSize(h * 0.01);
    os->drawRectangle(x, y, w, h);

    os->setFontSize(h * 0.05);
    os->buildVariableString(s->textBuffer, "x: %i     y: %i", (s32)s->displayPos.x, (s32)s->displayPos.y);
    os->drawText(s->textBuffer, w * 0.7, h * 0.05);
    s->displaySz = s->subspriteEndPos - s->subspriteStartPos;
    os->buildVariableString(s->textBuffer, "bx: %i", (s32)s->subspriteStartPos.x);
    os->drawText(s->textBuffer, w * 0.15, h * 0.1);
    os->buildVariableString(s->textBuffer, "by: %i", (s32)s->subspriteStartPos.y);
    os->drawText(s->textBuffer, w * 0.15, h * 0.2);
    os->buildVariableString(s->textBuffer, "bw: %i", (s32)s->displaySz.x);
    os->drawText(s->textBuffer, w * 0.15, h * 0.3);
    os->buildVariableString(s->textBuffer, "bh: %i", (s32)s->displaySz.y);
    os->drawText(s->textBuffer, w * 0.15, h * 0.4);
}

static void runAnimationPanel(f32 x, f32 y, f32 w, f32 h){
    Animation* a = s->currentAnimation;

    if(inBounds(os->mousePosition, x, y, w, h)){
        if(os->mouseScrollDelta){
            s->animationZoom += os->mouseScrollDelta * 0.1;
            s->animationZoom = max(s->animationZoom, 0.01);
        }
    }

    os->setColor2D(0, 0, 0, 1);
    os->setStrokeSize(h * 0.01);
    os->drawRectangle(x, y, w, h);

    if(a->totalFrames < 1){
        return;
    }

    
    Vector4 b = s->image.bounds;

    u32 i = a->currentFrame;
    s->image.bounds = a->frames[i];
    f32 iw = s->image.bounds.z * s->animationZoom;
    f32 ih = s->image.bounds.w * s->animationZoom;
    f32 ix = x + w * 0.5 - iw * 0.5;
    f32 iy = y + h * 0.5 - ih * 0.5;
    os->renderSprite2D(&s->image, ix, iy, iw, ih);
    s->image.bounds = b;
    
    if(s->playAnimation){
        a->frameTime += os->deltaTime;

        if(a->frameTime > a->delay){
            a->currentFrame = (a->currentFrame + 1) % a->totalFrames;
            a->frameTime -= a->delay;

        }
    }
}

static void runImageMapPanel(f32 x, f32 y, f32 w, f32 h){
    os->setColor2D(0, 0, 0, 1);
    os->setStrokeSize(h * 0.01);
    os->drawRectangle(x, y, w, h);

    if(s->image.bitmap){
        f32 bmw = (f32)s->image.width  * s->mapZoom;
        f32 bmh = (f32)s->image.height * s->mapZoom;
        f32 bmx = x + w * 0.5 - bmw * 0.5 + s->offset.x;
        f32 bmy = y + h * 0.5 - bmh * 0.5 + s->offset.y;

        f32 ix = clamp(bmx, x, x + w);
        f32 iy = clamp(bmy, y, y + h);
        f32 iw = bmw - max((x - bmx), 0);
        f32 ih = bmh - max((y - bmy), 0);

        s->image.bounds.x = max((x - bmx) / s->mapZoom, 0);
        s->image.bounds.y = max((y - bmy) / s->mapZoom, 0);
        s->image.bounds.z = clamp((s->image.width  - s->image.bounds.x), 0, s->image.width); 
        s->image.bounds.w = clamp((s->image.height - s->image.bounds.y), 0, s->image.height); 

        if(inBounds(os->mousePosition, x, y, w, h)){
            if(os->mouseButtons[os->MOUSE_BUTTON_MIDDLE]){
                s->offset += os->mousePosition - gui->previousMousePosition;
            }

            if(os->mouseScrollDelta){
                s->mapZoom += os->mouseScrollDelta * 0.1;
                s->mapZoom = max(s->mapZoom, 0.01);

                f32 nw = (f32)s->image.width  * s->mapZoom;
                f32 nx = x + w * 0.5 - nw * 0.5 + s->offset.x;
                f32 nh = (f32)s->image.height  * s->mapZoom;
                f32 ny = y + h * 0.5 - nh * 0.5 + s->offset.y;

                f32 pctx = map(os->mousePosition.x, bmx, bmx + bmw, 0, 1);
                pctx = clamp(pctx, 0, 1);
                pctx = map(pctx, 0, 1, bmx - nx, -(bmx - nx));
                f32 pcty = map(os->mousePosition.y, bmy, bmy + bmh, 0, 1);
                pcty = clamp(pcty, 0, 1);
                pcty = map(pcty, 0, 1, bmy - ny, -(bmy - ny));



                s->offset.x += pctx;
                s->offset.y += pcty;
            }

            if(inBounds(os->mousePosition, bmx, bmy, bmw, bmh)){
                s->displayPos = ((os->mousePosition - Vector2(bmx, bmy)) / s->mapZoom);
                s->displayPos = Vector2((u32)s->displayPos.x, (u32)s->displayPos.y);

                if(gui->leftMouseClickedOnce){
                    s->subspriteStartPos = s->displayPos;
                }else if(os->mouseButtons[os->MOUSE_BUTTON_LEFT]){
                    s->subspriteEndPos = s->displayPos + 1;
                }

                s->subspriteEndPos = clamp(s->subspriteEndPos, s->subspriteStartPos, Vector2(s->image.width, s->image.height));
            }
        }
        

        os->renderSprite2D(&s->image, ix, iy, iw, ih);

        os->setStrokeSize(h * 0.001);
        os->setColor2D(0, 0, 0, 1);
        os->drawRectangle(ix, iy, iw, ih);

        os->setColor2D(0, 1, 0, 1);

        f32 rx = s->subspriteStartPos.x * s->mapZoom;
        f32 ry = s->subspriteStartPos.y * s->mapZoom;
        f32 rw = s->subspriteEndPos.x * s->mapZoom - rx;
        f32 rh = s->subspriteEndPos.y * s->mapZoom - ry;

        if(rx + bmx < x){
            rw -= x - (rx + bmx);
            rx = x - bmx;
        }
        if(ry < y){
            rh -= y - (ry + bmy);
            ry = y - bmy;
        }   

        rw = max(rw, 0);
        rh = max(rh, 0);

        os->drawRectangle(rx + bmx, ry + bmy, rw, rh);
    }
}

static void initializeApplication(Spranimator* sp){
    s = sp;
    os = s->os;

    s->mapZoom = 1;
    s->animationZoom = 1;
    s->currentAnimation = &s->animations[0];
}

extern "C" void updateApplication(Spranimator* sp){
    s = sp;
    os = s->os;

    os->begin2DRender();
    initializeGUI(&s->gui);

    os->clearRenderTarget2D(0, 0, 0, 1);

    os->setColor2D(0.5, 0.55, 0.6, 1);
    os->fillRectangle(0, 0, os->windowResolution.x, os->windowResolution.y);

    runControllPanel(0, 0, PH(0.5), PV(0.5));
    runImageMapPanel(PH(0.5), 0, PH(0.5), PV(1));
    runAnimationPanel(0, PV(0.5), PH(0.5), PV(0.5));

    finalizeGUI();
    os->end2DRender();
}