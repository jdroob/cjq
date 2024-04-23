/* io.c */
#include <stdio.h>
#include <stdint.h>

void _print_int(int x) {
  printf("out: %i\n",x);
}

void _print_uint16_t(uint16_t u) {
  printf("out: %i\n", u);
}

void _print_str(const char* s) {
  // printf("out: %i\n", &s[0]); // Debug
  printf("%s", s);
}