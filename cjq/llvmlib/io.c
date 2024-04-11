/* io.c */
#include <stdio.h>

void _print_int(int x) {
  printf("out: %i\n",x);
}

void _print_str(const char* s) {
  printf("HERE\n");
  printf("out: %i\n", &s[0]);
  printf("%s", s);
}