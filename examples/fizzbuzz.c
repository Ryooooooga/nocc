int puts(const char *s);
int printf(const char *f, ...);

void fizzbuzz(int n) {
    int i;
    for (i = 1; i <= n; i++) {
        if (i % 15 == 0) {
            puts("Fizz Buzz");
        } else if (i % 3 == 0) {
            puts("Fizz");
        } else if (i % 5 == 0) {
            puts("Buzz");
        } else {
            printf("%d\n", i);
        }
    }
}

int main(void) {
    fizzbuzz(50);
    return 0;
}
