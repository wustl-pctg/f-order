#ifndef _UTIL_H
#define _UTIL_H

void print_bits(label_t num) {
  label_t size = sizeof(label_t);
  label_t max_pow = (label_t)1 << (size * 8 - 1);

  for (int i = 0; i < size * 8; ++i) {
    // print last bit and shift left.
    printf("%lu ", (label_t)!!(num & max_pow));
    //    num <<= 1;
    max_pow >>= 1;
  }
  printf("\n");
}

#endif // ifndef _UTIL_H
