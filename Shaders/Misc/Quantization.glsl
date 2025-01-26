
#ifndef _QUANTIZATION_
#define _QUANTIZATION_

uint quantizeFloatToU8(float f, float offs, float scale) {
  f -= offs;
  f /= scale;
  f = clamp(f, 0.0, 1.0);
  return clamp(uint(f * 256.0), 0, 255);
}

float dequantizeU8ToFloat(uint u, float offs, float scale) {
  float f = float(u & 0xFF) / 256.0;
  f *= scale;
  f += offs;
  return f;
}

#endif // _QUANTIZATION_
