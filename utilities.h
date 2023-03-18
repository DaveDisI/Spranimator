#pragma once

#include <stdarg.h>

#define MAX_U32     4294967295
#define MAX_F32     3.402823466e38
#define MIN_F32     -1.175494351e38     

#define KILOBYTE(x) (x * 1024)
#define MEGABYTE(x) (x * KILOBYTE(1024))
#define GIGABYTE(x) (x * MEGABYTE(1024))
#define TERABYTE(x) (x * GIGABYTE(1024))

#define RI_PTR(PTR, TYPE) *(TYPE*)PTR; PTR += sizeof(TYPE)
#define WI_PTR(PTR, VAL, TYPE) *(TYPE*)PTR = VAL; PTR += sizeof(TYPE)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;
typedef float f32;
typedef double f64;

static u32 getLength(s8* str){
    u32 res = 0;
    s8* c = str;
    while(*c++ != 0) res++;
    return res;
}

static f32 stringToF32(const s8* str){
    float result = 0;
    float mult = 1;
    const s8* c = str;
    if(*c == '-'){
        c++;
        mult = -1;
    }
    bool decFound = false;
    while(*c != '\0'){
        if (*c == '.'){
            decFound = true; 
            c++;
            continue;
        }
        u32 d = *c - '0';
        if(d >= 0 && d <= 9){
            if(decFound) {
                mult /= 10.0f;
            }
            result = result * 10.0f + (float)d;
        }
        c++;
    }

    return result * mult;
}

static u32 stringToU32(const s8* str){
    u32 result = 0;
    u32 mul = 1;
    const s8* c = str;
    while(*c){
        c++;
    }
    c++;
    while(c >= str){
        if(*c >= '0' && *c <= '9'){
            result += (*c - '0') * mul;
            mul *= 10;
        }
        c--;
    }

    return result;
}

static void copyString(s8* s1, s8* s2){
    while(*s2){
        *s1++ = *s2++;
    }
    *s1 = 0;
}

static void copyString(s8* s1, s8* s2, u32 len){
    while(len){
        *s1++ = *s2++;
        len--;
    }
}

static void appendString(s8* s1, s8* s2){
    while(*s1){
        s1++;
    }
    while(*s2){
        *s1++ = *s2++;
    }
    *s1 = '\0';
}

static bool compareStrings(s8* s1, s8* s2){
    while((*s1 || *s2) && (*s1 == *s2)){
        s1++;
        s2++;
    }

    return *s1 == *s2;
}
