#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    srand(time(NULL));

    // Generate and print a random number between 0 and 999
    int random_number = rand() % 1000;
    printf("%d\n", random_number);

    return 0;
}
