//
// Created by Dottik on 28/11/2024.
//

#pragma once
#include "imgui.h"

struct Color4 {
    float r, g, b, a;

    static Color4 FromRGB(float r, float g, float b) {
        return {
            r / 255,
            g / 255,
            b / 255,
            1
        };
    }

    static Color4 FromRGBNormalized(float r, float g, float b) {
        return {
            r, g, b, 1
        };
    }

    ImVec4 ToImGuiVec4() {
        return {
            r,
            g,
            b,
            a
        };
    }
};
