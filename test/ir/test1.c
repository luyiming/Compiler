int main() {
    // int i = 1;
    // int j = 2;
    // int k = i + j * 2;
    // k = read();
    // write(k);
    int i, j, k;
    i = read();
    j = read();
    if (i == j) {
        write(1);
    } else if (i < j) {
        write(2);
    } else {
        write(3);
    }
    return 2;
}
