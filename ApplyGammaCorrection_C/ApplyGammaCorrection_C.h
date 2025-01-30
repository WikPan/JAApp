#ifndef APPLY_GAMMA_CORRECTION_C_H
#define APPLY_GAMMA_CORRECTION_C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	__declspec(dllexport) void ApplyGammaCorrection_C(uint8_t* imageData, int pixelCount, uint8_t* LUT);

#ifdef __cplusplus
}
#endif

#endif // APPLY_GAMMA_CORRECTION_C_H
