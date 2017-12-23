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
struct B {
    struct A q;
};

struct A c[2][2], d;
struct B w;

int main() {
    // int i = 1;
    // int j = 2;
    // int k = i + j * 2;
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

    // int a[2][3][4];
    // int b[2][7];
    // b[1][2] = 3;
    // return a[1][1][1];

    // d.x = c[1][1].x;
    // d.y[2] = 1;
    // c[1][1].y[1] = 2;
    return w.q.x + w.q.y[1];
}
