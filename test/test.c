#include <stdio.h>
int main() {
    struct A{
        int d;
    };
    int d = 1;
    if (d == 1) {
      struct A {
        int d;
      };
      struct A a;
      printf("%d\n", a.d);
    }
}