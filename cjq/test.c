/* test.c */
#include <stdio.h>

extern int test();

int main() {
  printf("test() returned %i\n", test());
}