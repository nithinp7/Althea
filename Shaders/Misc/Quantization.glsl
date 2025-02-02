
#ifndef _QUANTIZATION_
#define _QUANTIZATION_

uint quantizeFloatToU8(float f, float offs, float scale) {
  f -= offs;
  f /= scale;
  f = clamp(f, 0.0, 1.0);
  return clamp(uint(round(f * 255.0)), 0, 255);
}

float dequantizeU8ToFloat(uint u, float offs, float scale) {
  float f = float(u & 0xFF) / 255.0;
  f *= scale;
  f += offs;
  return f;
}

uint quantizeFloatToU16(float f, float offs, float scale) {
  f -= offs;
  f /= scale;
  f = clamp(f, 0.0, 1.0);
  return clamp(uint(round(f * float(0xFFFF))), 0, 0xFFFF);
}

float dequantizeU16ToFloat(uint u, float offs, float scale) {
  float f = float(u & 0xFFFF) / float(0xFFFF);
  f *= scale;
  f += offs;
  return f;
}

#endif // _QUANTIZATION_
