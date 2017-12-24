int quicksort(int number[5], int first, int last) {
    int i, j, pivot, temp;

    if (first < last) {
        pivot = first;
        i = first;
        j = last;

        while (i < j) {
            while (number[i] <= number[pivot] && i < last) 
                i = i + 1;
            while (number[j] > number[pivot]) j = j - 1;
            if (i < j) {
                temp = number[i];
                number[i] = number[j];
                number[j] = temp;
            }
        }

        temp = number[pivot];
        number[pivot] = number[j];
        number[j] = temp;
        quicksort(number, first, j - 1);
        quicksort(number, j + 1, last);
    }

    return 0;
}

int main() {
    int k, data[5];

    data[0] = 5;
    data[1] = 4;
    data[2] = 2;
    data[3] = 1;
    data[4] = 3;

    quicksort(data, 0, 4);

    k = 0;
    while (k < 5) {
        write(data[k]);
        k = k + 1;
    }

    return 0;
}