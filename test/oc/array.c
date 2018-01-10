int main() {
    int a[10], i = 2;
    a[0] = 1;
    a[1] = 1;
    while (i < 10) {
        a[i] = a[i - 1] + a[i - 2];
        i = i + 1;
    }
    i = 0;
    while (i < 10) {
        write(a[i]);
        i = i + 1;
    }
    return 0;
}