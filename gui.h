#pragma once

#include "os_interface.h"
#include "mathematics.h"

#define PCT5H(X, Y, W, H, L) (X) * os->windowResolution.x, (Y) * os->windowResolution.y, (W) * os->windowResolution.x, (H) * os->windowResolution.y, (L) * os->windowResolution.y
#define PCT5V(X, Y, W, H, L) (X) * os->windowResolution.x, (Y) * os->windowResolution.y, (W) * os->windowResolution.x, (H) * os->windowResolution.y, (L) * os->windowResolution.x 
#define PCT4(X, Y, W, H) (X) * os->windowResolution.x, (Y) * os->windowResolution.y, (W) * os->windowResolution.x, (H) * os->windowResolution.y 
#define PCT2(X, Y) (X) * os->windowResolution.x, (Y) * os->windowResolution.y 
#define PCT1H(X) (X) * os->windowResolution.x
#define PCT1V(X) (X) * os->windowResolution.y

struct GUI {
    s8 inputBuffers[256][256];
    Vector2 lastLeftClickedPosition;
    Vector2 lastRightClickedPosition;
    Vector2 previousMousePosition;
    u32 totalInputs;
    u32 activeInput;
    bool inputClicked;

    bool leftMouseClickedOnce;
    bool rightMouseClickedOnce;
    bool leftMouseReleased;
    bool rightMouseReleased;
    bool lastLeftMouse;
    bool lastRightMouse;

    bool textHighlight;
};
static GUI* gui;


static bool inBounds(Vector2 p, f32 x, f32 y, f32 w, f32 h){
    return p.x > x && p.x < x + w && p.y > y && p.y < y + h;
}


static void initializeGUI(GUI* g){
    gui = g;
    gui->totalInputs = 0;

    if(os->mouseButtons[os->MOUSE_BUTTON_LEFT] && !gui->lastLeftMouse){
        gui->leftMouseClickedOnce = true;
        gui->leftMouseReleased = false;
    }else if(!os->mouseButtons[os->MOUSE_BUTTON_LEFT] && gui->lastLeftMouse){        
        gui->leftMouseReleased = true;
        gui->leftMouseClickedOnce = false;
    }else{
        gui->leftMouseClickedOnce = false;
        gui->leftMouseReleased = false;
    }

    if(os->mouseButtons[os->MOUSE_BUTTON_RIGHT] && !gui->lastRightMouse){
        gui->rightMouseClickedOnce = true;
        gui->rightMouseReleased = false;
    }else if(!os->mouseButtons[os->MOUSE_BUTTON_RIGHT] && gui->lastRightMouse){
        gui->rightMouseReleased = true;
        gui->rightMouseClickedOnce = false;
    }else{
        gui->rightMouseClickedOnce = false;
        gui->rightMouseReleased = false;
    }

}

static void finalizeGUI(){
    if(gui->activeInput == (u32)-1){
        os->charTypedFlag = false;
    }

    if(gui->leftMouseClickedOnce){
        gui->lastLeftClickedPosition = os->mousePosition;
    }
    if(gui->rightMouseClickedOnce){
        gui->lastRightClickedPosition = os->mousePosition;
    }
    gui->lastLeftMouse = os->mouseButtons[os->MOUSE_BUTTON_LEFT];
    gui->lastRightMouse = os->mouseButtons[os->MOUSE_BUTTON_RIGHT];
    gui->previousMousePosition = os->mousePosition;

    if(gui->totalInputs && gui->activeInput != (u32)-1){
        gui->activeInput %= gui->totalInputs;
    }
}

static void centeredText(s8* text, f32 y, f32 x1, f32 x2){
    f32 tw;
    f32 th;
    os->getTextDimensions(text, &tw, &th);
    os->drawText(text, x1 + ((x2 - x1)  * 0.5) - tw * 0.5, y);
}

static bool checkBox(bool* v, f32 x, f32 y, f32 w, f32 h){
    f32 c1[3] = {0.8, 0.8, 0.8};
    bool drawX = false;
    bool clicked = false;
    if(inBounds(os->mousePosition, x, y, w, h)){
        c1[0] = 0.9;
        c1[1] = 0.9;
        c1[2] = 0.9;
        
        bool bds = inBounds(gui->lastLeftClickedPosition, x, y, w, h);
        if(os->mouseButtons[os->MOUSE_BUTTON_LEFT] && bds){
            if(!*v){
                drawX = true;
            }
        }else if(gui->lastLeftMouse && bds){
            *v = !*v;
            clicked = true;
            drawX = *v;
        }else if(*v){
            drawX = true;
        }
    }else if(!os->mouseButtons[os->MOUSE_BUTTON_LEFT] && gui->lastLeftMouse){
        drawX = true;
    }else if(*v){
        drawX = true;
    }


    os->setColor2D(c1[0], c1[1], c1[2], 9);
    os->fillRectangle(x, y, w, h);
    os->setColor2D(0.7, 0.7, 0.7, 1);
    os->setStrokeSize(h * 0.1);
    os->drawLine(x, y, x, y + h);
    os->drawLine(x, y + h, x + w, y + h);
    os->setColor2D(0.3, 0.3, 0.3, 1);
    os->drawLine(x + w, y + h, x + w, y);
    os->drawLine(x, y, x + w, y);

    if(drawX){
        f32 d = h * 0.2;
        os->setColor2D(0.1, 0.1, 0.1, 1);
        os->setStrokeSize(d);
        os->drawLine(x + d, y + d, x + w - d, y + h - d);
        os->drawLine(x + w - d, y + d, x + d, y + h - d);
    }

    return clicked;
}

static bool textInput(s8* text, f32 x, f32 y, f32 w, f32 h){
    f32 color1[3] = {0.9, 0.9, 0.9};
    f32 color2[3] = {0.2, 0.2, 0.2};

    bool active = gui->activeInput == gui->totalInputs;
    bool result = false;
    if(active){
        if(os->charTypedFlag){
            os->charTypedFlag = false;
            u32 ctr = getLength(text);
            if(os->lastTypedChar == os->KEY_BACKSPACE){
                if(gui->textHighlight){
                    text[0] = '\0';
                    ctr = 0;
                    gui->textHighlight = false;
                }else if(ctr > 0){
                    text[ctr - 1] = '\0';
                }
            }else if(os->lastTypedChar == os->KEY_ENTER){
                result = true;
                gui->textHighlight = false;
                gui->activeInput = -1;
            }else if(os->lastTypedChar == os->KEY_TAB){
                result = true;
                gui->textHighlight = false;
                gui->activeInput++;
            }else{
                if(gui->textHighlight){
                    text[0] = os->lastTypedChar;
                    text[1] = '\0';
                    ctr = 1;
                    gui->textHighlight = false;
                }else{
                    text[ctr] = os->lastTypedChar;
                    text[ctr + 1] = '\0';
                    ctr++;
                }
            }
        }

        if(gui->leftMouseClickedOnce){
            if(!inBounds(os->mousePosition, x, y, w, h)){
                result = true;
                gui->textHighlight = false;
                gui->activeInput = -1;
            }
        }
    }

    if(gui->leftMouseClickedOnce){
        if(inBounds(os->mousePosition, x, y, w, h)){
            if(inBounds(gui->lastLeftClickedPosition, x, y, w, h)){
                gui->textHighlight = !gui->textHighlight;
            }else{
                gui->textHighlight = true;
            }
        }
    }

    if(gui->leftMouseClickedOnce && !gui->inputClicked){
        if(inBounds(os->mousePosition, x, y, w, h)){
            gui->activeInput = gui->totalInputs;
            gui->inputClicked = true;
        }
    }else if(!os->mouseButtons[os->MOUSE_BUTTON_LEFT]){
        gui->inputClicked = false;
    }

    f32 ssz = h * 0.1;
    f32 hssz = ssz * 0.5;
    f32 tw, th;
    os->setStrokeSize(ssz);
    os->setFontSize(h * 0.75);
    os->getTextDimensions(text, &tw, &th);
    os->setColor2D(0.1, 0.1, 0.1, 1);
    os->fillRectangle(x, y, w, h);
    os->setColor2D(0.9, 0.9, 0.9, 1);
    f32 tx = x + (w / 2) - tw / 2;
    f32 ty = y + (h / 2) - th / 2;
    if(active && (u32)(os->elapsedTime * 2) % 2){
        os->fillRectangle(tx + tw, y, 5, h);
    }
    os->drawText(text, tx, ty);
    os->setColor2D(color1[0], color1[1], color1[2], 1);
    os->drawLine(x, y, x, y + h);
    os->drawLine(x - hssz, y + h, x + w, y + h);
    os->setColor2D(color2[0], color2[1], color2[2], 1);
    os->drawLine(x + w, y + h, x + w, y);
    os->drawLine(x, y, x + w + hssz, y);
    
    if(active && gui->textHighlight){
        os->setColor2D(1, 1, 1, 0.5);
        os->fillRectangle(x, y, w, h);
    }

    gui->totalInputs++;

    return result;
}

static bool f32Input(f32* v, f32 x, f32 y, f32 w, f32 h){
    s8* buffer = gui->inputBuffers[gui->totalInputs];
    u32 bufLen = getLength(buffer);

    f32 color1[3] = {0.9, 0.9, 0.9};
    f32 color2[3] = {0.2, 0.2, 0.2};

    bool active = gui->activeInput == gui->totalInputs;
    bool result = false;
    if(active){
        if(os->charTypedFlag){
            if((os->lastTypedChar >= '0' && os->lastTypedChar <= '9') ||
                os->lastTypedChar == '-' || os->lastTypedChar == '+'  ||
                os->lastTypedChar == '.'){

                if(gui->textHighlight){
                    buffer[0] = os->lastTypedChar;
                    buffer[1] = '\0';
                    bufLen = 1;
                    gui->textHighlight = false;
                }else{
                    buffer[bufLen++] = os->lastTypedChar;
                    buffer[bufLen] = '\0';
                }
            }else if(os->lastTypedChar == os->KEY_BACKSPACE){
                if(gui->textHighlight){
                    buffer[0] = '\0';
                    bufLen = 0;
                    gui->textHighlight = false;
                }else{
                    if(bufLen > 0){
                        buffer[--bufLen] = '\0';
                    }else{
                        *v = 0;
                        buffer[0] = '\0';
                    }
                }
            }else if(os->lastTypedChar == os->KEY_ENTER){
                result = true;
                *v = stringToF32(buffer);
                os->buildVariableString(buffer, "%.2f", *v);
                gui->textHighlight = false;
                gui->activeInput = -1;
            }
            else if(os->lastTypedChar == os->KEY_TAB){
                result = true;
                *v = stringToF32(buffer);
                os->buildVariableString(buffer, "%.2f", *v);
                gui->textHighlight = true;
                gui->activeInput++;
            }

            os->charTypedFlag = false;
        }

        if(gui->leftMouseClickedOnce && !inBounds(os->mousePosition, x, y, w, h)){
            result = true;
            *v = stringToF32(buffer);
            os->buildVariableString(buffer, "%.2f", *v);
            gui->textHighlight = false;
            gui->activeInput = -1;
        }
    }else if(bufLen == 0 || *v != stringToF32(buffer)){
        os->buildVariableString(buffer, "%.2f", *v);
    }

    if(gui->leftMouseClickedOnce){
        if(inBounds(os->mousePosition, x, y, w, h)){
            if(inBounds(gui->lastLeftClickedPosition, x, y, w, h)){
                gui->textHighlight = !gui->textHighlight;
            }else{
                gui->textHighlight = true;
            }
        }
    }

    if(gui->leftMouseClickedOnce && !gui->inputClicked){
        if(inBounds(os->mousePosition, x, y, w, h)){
            gui->activeInput = gui->totalInputs;
            gui->inputClicked = true;        
        }
    }else if(!os->mouseButtons[os->MOUSE_BUTTON_LEFT]){
        gui->inputClicked = false;
    }

    f32 ssz = h * 0.1;
    f32 hssz = ssz * 0.5;
    f32 tw, th;
    os->setStrokeSize(h * 0.1);
    os->setFontSize(h * 0.75);
    os->getTextDimensions(buffer, &tw, &th);
    os->setColor2D(0.1, 0.1, 0.1, 1);
    os->fillRectangle(x, y, w, h);
    os->setColor2D(0.9, 0.9, 0.9, 1);
    f32 tx = x + (w / 2) - tw / 2;
    f32 ty = y + (h / 2) - th / 2;
    if(active && (u32)(os->elapsedTime * 2) % 2){
        os->fillRectangle(tx + tw, y, 5, h);
    }
    
    os->drawText(buffer, tx, ty);
    

    os->setColor2D(color1[0], color1[1], color1[2], 1);
    os->drawLine(x, y, x, y + h);
    os->drawLine(x - hssz, y + h, x + w, y + h);
    os->setColor2D(color2[0], color2[1], color2[2], 1);
    os->drawLine(x + w, y + h, x + w, y);
    os->drawLine(x, y, x + w + hssz, y);
    
    if(active && gui->textHighlight){
        os->setColor2D(1, 1, 1, 0.5);
        os->fillRectangle(x, y, w, h);
    }

    gui->totalInputs++;

    return result;
}

static bool u32Input(u32* v, f32 x, f32 y, f32 w, f32 h){
    s8* buffer = gui->inputBuffers[gui->totalInputs];
    u32 bufLen = getLength(buffer);

    f32 color1[3] = {0.9, 0.9, 0.9};
    f32 color2[3] = {0.2, 0.2, 0.2};

    bool active = gui->activeInput == gui->totalInputs;
    bool result = false;
    if(active){
        if(os->charTypedFlag){
            if(os->lastTypedChar >= '0' && os->lastTypedChar <= '9'){
                if(gui->textHighlight){
                    buffer[0] = os->lastTypedChar;
                    buffer[1] = '\0';
                    bufLen = 0;
                    gui->textHighlight = false;
                }else{
                    buffer[bufLen++] = os->lastTypedChar;
                    buffer[bufLen] = '\0';
                }
            }else if(os->lastTypedChar == os->KEY_BACKSPACE){
                if(gui->textHighlight){
                    buffer[0] = '\0';
                    bufLen = 0;
                    gui->textHighlight = false;
                }else{
                    if(bufLen > 0){
                        buffer[--bufLen] = '\0';
                    }else{
                        *v = 0;
                        buffer[0] = '\0';
                    }
                }
            }else if(os->lastTypedChar == os->KEY_ENTER){
                result = true;
                *v = stringToU32(buffer);
                os->buildVariableString(buffer, "%u", *v);
                gui->activeInput = -1;
                gui->textHighlight = false;
            }else if(os->lastTypedChar == os->KEY_TAB){
                result = true;
                *v = stringToU32(buffer);
                os->buildVariableString(buffer, "%u", *v);
                gui->textHighlight = true;
                gui->activeInput++;
            }
            os->charTypedFlag = false;
        }

        if(gui->leftMouseClickedOnce && !inBounds(os->mousePosition, x, y, w, h)){
            result = true;
            *v = stringToF32(buffer);
            os->buildVariableString(buffer, "%u", *v);
            gui->textHighlight = false;
            gui->activeInput = -1;
        }
    }else if(bufLen == 0 || *v != stringToU32(buffer)){
        os->buildVariableString(buffer, "%u", *v);
    }

    if(gui->leftMouseClickedOnce){
        if(inBounds(os->mousePosition, x, y, w, h)){
            if(inBounds(gui->lastLeftClickedPosition, x, y, w, h)){
                gui->textHighlight = !gui->textHighlight;
            }else{
                gui->textHighlight = true;
            }
        }
    }

    if(gui->leftMouseClickedOnce && !gui->inputClicked){
        if(inBounds(os->mousePosition, x, y, w, h)){
            gui->activeInput = gui->totalInputs;
            gui->inputClicked = true;
        }
    }else if(!os->mouseButtons[os->MOUSE_BUTTON_LEFT]){
        gui->inputClicked = false;
    }

    f32 ssz = h * 0.1;
    f32 hssz = ssz * 0.5;
    f32 tw, th;
    os->setStrokeSize(h * 0.1);
    os->setFontSize(h * 0.75);
    os->getTextDimensions(buffer, &tw, &th);
    os->setColor2D(0.1, 0.1, 0.1, 1);
    os->fillRectangle(x, y, w, h);
    os->setColor2D(0.9, 0.9, 0.9, 1);
    f32 tx = x + (w / 2) - tw / 2;
    f32 ty = y + (h / 2) - th / 2;
    if(active && (u32)(os->elapsedTime * 2) % 2){
        os->fillRectangle(tx + tw, y, 5, h);
    }
    
    os->drawText(buffer, tx, ty);
    

    os->setColor2D(color1[0], color1[1], color1[2], 1);
    os->drawLine(x, y, x, y + h);
    os->drawLine(x - hssz, y + h, x + w, y + h);
    os->setColor2D(color2[0], color2[1], color2[2], 1);
    os->drawLine(x + w, y + h, x + w, y);
    os->drawLine(x, y, x + w + hssz, y);
    
    if(active && gui->textHighlight){
        os->setColor2D(1, 1, 1, 0.5);
        os->fillRectangle(x, y, w, h);
    }

    gui->totalInputs++;

    return result;
}

static void horizontalSlider(f32 x, f32 y, f32 w, f32 h, f32 l, f32* v){
    f32 bx = x + *v * l;
    f32 by = y - h / 2;
    os->setStrokeSize(5);
    os->setColor2D(0.2, 0.2, 0.2, 1);
    os->drawLine(x, y, x + l, y);
    os->setStrokeSize(2);
    os->setColor2D(0.8, 0.8, 0.8, 1);
    os->drawLine(x, y + 2, x + l, y + 2);
    
    
    if(os->mouseButtons[os->MOUSE_BUTTON_LEFT] && 
       inBounds(os->mousePosition, x, by, l, h) && 
       inBounds(gui->lastLeftClickedPosition, x, by, l, h)){
        f32 dif = os->mousePosition.x - x;
        *v = dif / l;
        *v = clamp(*v, 0, 1);
    }
    
    bx = (x + *v * l) - w / 2;

    if(inBounds(os->mousePosition, bx, by, w, h)){
        os->setColor2D(0.9, 0.9, 0.9, 1);
    }else{
        os->setColor2D(0.7, 0.7, 0.7, 1);
    }
    os->fillRectangle(bx, by, w, h);
    os->setStrokeSize(2);
    os->setColor2D(0.2, 0.2, 0.2, 1);
    os->drawLine(bx, by + h, bx + w, by + h);
    os->drawLine(bx, by, bx, by + h);
    os->setColor2D(0.9, 0.9, 0.9, 1);
    os->drawLine(bx, by, bx + w, by);
    
    os->drawLine(bx + w, by, bx + w, by + h);
}

static void verticalSlider(f32 x, f32 y, f32 w, f32 h, f32 l, f32* v){
    f32 bx = x - w / 2;
    f32 by;
    os->setStrokeSize(5);
    os->setColor2D(0.2, 0.2, 0.2, 1);
    os->drawLine(x, y, x, y - l);
    os->setStrokeSize(2);
    os->setColor2D(0.8, 0.8, 0.8, 1);
    os->drawLine(x-2, y, x-2, y - l);
    
    if(os->mouseButtons[os->MOUSE_BUTTON_LEFT] && inBounds(os->mousePosition, bx, y - l, w, l)){
        f32 dif = y - os->mousePosition.y;
        *v = dif / l;
    }
    *v = clamp(*v, 0, 1);
    by = (y - *v * l) - (h / 2);
    if(inBounds(os->mousePosition, bx, by, w, h)){
        os->setColor2D(0.9, 0.9, 0.9, 1);
    }else{
        os->setColor2D(0.7, 0.7, 0.7, 1);
    }
    os->fillRectangle(bx, by, w, h);
    os->setStrokeSize(2);
    os->setColor2D(0.2, 0.2, 0.2, 1);
    os->drawLine(bx, by + h, bx + w, by + h);
    os->drawLine(bx, by, bx, by + h);
    os->setColor2D(0.9, 0.9, 0.9, 1);
    os->drawLine(bx, by, bx + w, by);
    
    os->drawLine(bx + w, by, bx + w, by + h);
}

static void verticalSlider(f32 x, f32 y, f32 w, f32 h, f32 l, f32* v, f32 min, f32 max){
    f32 tv = map(*v, min, max, 0, 1);
    verticalSlider(x, y, w, h, l, &tv);
    *v = map(tv, 0, 1, min, max);
}

static bool button(s8* text, f32 x, f32 y, f32 w, f32 h){
    bool result = false;

    f32 color1[3] = {0.2, 0.2, 0.2};
    f32 color2[3] = {0.9, 0.9, 0.9};
    f32 color[3] = {0.7, 0.7, 0.7};
    if(inBounds(os->mousePosition, x, y, w, h)){
        color[0] = 0.8;
        color[1] = 0.8;
        color[2] = 0.8;

        if(os->mouseButtons[os->MOUSE_BUTTON_LEFT]){
            if(inBounds(gui->lastLeftClickedPosition, x, y, w, h)){
                color[0] = 0.7;
                color[1] = 0.7;
                color[2] = 0.7;
                color1[0] = 0.9;
                color1[1] = 0.9;
                color1[2] = 0.9;
                color2[0] = 0.2;
                color2[1] = 0.2;
                color2[2] = 0.2;
            }
        }else if(gui->leftMouseReleased){
            if(inBounds(gui->lastLeftClickedPosition, x, y, w, h)){
                result = true;
                gui->lastLeftClickedPosition = Vector2(-1, -1);
            }
        }
    }

    f32 ssz = h * 0.05;
    f32 hssz = ssz * 0.5;
    f32 tw, th;
    os->setStrokeSize(h * 0.1);
    os->setFontSize(h * 0.33);
    os->getTextDimensions(text, &tw, &th);
    os->setColor2D(color[0], color[1], color[2], 1);
    os->fillRectangle(x, y, w, h);
    os->setStrokeSize(ssz);
    os->setColor2D(color1[0], color1[1], color1[2], 1);
    os->drawLine(x, y, x, y + h);
    os->drawLine(x - hssz, y + h, x + w, y + h);
    os->setColor2D(color2[0], color2[1], color2[2], 1);
    os->drawLine(x + w, y + h, x + w, y);
    os->drawLine(x, y, x + w + hssz, y);

    os->setColor2D(0, 0, 0, 1);
    os->drawText(text, x + (w / 2) - tw / 2, y + (h / 2) - th / 2);

    return result;
}

void toggleSwitch(s8** strings, u32* toggle, u32 count, f32 x, f32 y, f32 w, f32 h){
    if(button(strings[*toggle], x, y, w, h)){
        *toggle = (*toggle + 1) % count;
    }
    os->setColor2D(0, 1, 0, 0.25);
    os->fillRectangle(x, y, w, h);
}