#include <stdint.h>
#include "ApplyGammaCorrection_C.h"

__declspec(dllexport) void ApplyGammaCorrection_C(uint8_t* imageData, int pixelCount, uint8_t* LUT) {
    for (int i = 0; i < pixelCount; ++i) {
        imageData[3 * i] = LUT[imageData[3 * i]];      // Czerwony kana³
        imageData[3 * i + 1] = LUT[imageData[3 * i + 1]];  // Zielony kana³
        imageData[3 * i + 2] = LUT[imageData[3 * i + 2]];  // Niebieski kana³
    }
}
