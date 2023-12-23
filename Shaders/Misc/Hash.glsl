#ifndef _HASH_
#define _HASH_

// From Mathias Muller
uint hashCoords(int x, int y, int z) {
  return uint(abs((x * 92837111) ^ (y * 689287499) ^ (z * 283923481)));
}

#endif // _HASH_
