#ifndef APPLY_GAMMA_CORRECTION_C_H
#define APPLY_GAMMA_CORRECTION_C_H

#include <stdint.h>

__declspec(dllexport) void ApplyGammaCorrection_C(uint8_t* imageData, int pixelCount, uint8_t* LUT);

#endif // APPLY_GAMMA_CORRECTION_C_H
