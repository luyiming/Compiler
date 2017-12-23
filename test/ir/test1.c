// int func(int a, int b, int c) {
//     a = 1;
//     b = 2;
//     c = 3;
//     return 1;
// }

struct A {
    int x;
    int y[3];
};

struct A c[2][2][2];

int main() {
    int i = 1;
    int j = 2;
    int k = i + j * 2;
    // k = read();
    // write(k);

    // int i, j, k;
    // i = read();
    // j = read();
    // if (i == j) {
    //     write(1);
    // } else if (i < j) {
    //     write(2);
    // } else {
    //     write(3);
    // }

    // func(6, 7, 8);

    int a[2][3][4];
    int b[2][7];
    b[1][2] = 3;
    return a[1][1][1];
}
