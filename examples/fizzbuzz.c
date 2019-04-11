int putchar(int c);
int puts(const char *s);

void print_int(int n) {
    if (n >= 10) {
        print_int(n / 10);
    }

    putchar(48 + n % 10);
}

void fizzbuzz(int n) {
    int i;
    for (i = 1; i <= n; i = i + 1) {
        if (i % 15 == 0) {
            puts("Fizz Buzz");
        } else if (i % 3 == 0) {
            puts("Fizz");
        } else if (i % 5 == 0) {
            puts("Buzz");
        } else {
            print_int(i);
            putchar(10);
        }
    }
}

int main(void) {
    fizzbuzz(50);
    return 0;
}
