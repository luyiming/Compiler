
int main() {
    int i = 1, j = 2, k = 2;
    if (i == j) {
        write(1);
    } else if (i < j) {
        if (j == k) {
            write(2);
        } else {
            write(3);
        }
    } else {
        write(4);
    }
    return 2;
}
