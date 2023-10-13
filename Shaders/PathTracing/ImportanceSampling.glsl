
// Given two distributions A and B, we importance sample f from A and g from B.
float MisBalanceHeuristic(float f, float pdf_f, float g, float pdf_g) {

  // For now nf and ng are 1 (one sample from each distribution)
  // In the future maybe we'll want to prioritize more samples from
  // one distribution or even combine spatially/temporally neighboring
  // samples.

  // Beta is 1.0 in balance heuristic

  // TODO: 
  // Check if either pdf is zero before this...

  float pdfSum = pdf_f + pdf_g;
  return f / pdfSum + g / pdfSum;
}

float MisPowerHeuristic(float f, float pdf_f, float g, float pdf_g) {
  float pdfSumSq = pdf_f * pdf_f + pdf_g * pdf_g;
  return f * pdf_f / pdfSumSq + g * pdf_g / pdfSumSq;
}