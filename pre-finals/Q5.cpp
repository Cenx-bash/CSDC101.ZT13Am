#include <iostream>
using namespace std;

void bubbleSort(int* arr, int n) {
    for (int* i = arr; i < arr + n - 1; i++) {
        for (int* j = arr; j < arr + n - 1; j++) {
            if (*j > *(j + 1)) {
                int temp = *j;
                *j = *(j + 1);
                *(j + 1) = temp;
            }
        }
    }
}

int main() {
    int arr[5];

    for (int i = 0; i < 5; i++) cin >> arr[i];

    bubbleSort(arr, 5);

    for (int i = 0; i < 5; i++) cout << arr[i] << " ";
}
