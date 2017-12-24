int main() {
    int i = 0, sum = 0;
    while (i <= 10) {
        sum = sum + i;
        i = i + 1;
    }
    write(sum);
    return 0;
}