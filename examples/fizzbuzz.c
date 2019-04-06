int putchar(int c);

void print_fizz(void) {
    putchar(64 + 6);
    putchar(96 + 9);
    putchar(96 + 26);
    putchar(96 + 26);
}

void print_buzz(void) {
    putchar(64 + 2);
    putchar(96 + 21);
    putchar(96 + 26);
    putchar(96 + 26);
}

void print_fizzbuzz(void) {
    print_fizz();
    putchar(32);
    print_buzz();
}

void print_int(int n) {
    if (n >= 10) {
        print_int(n / 10);
    }

    putchar(48 + n % 10);
}

void fizzbuzz(int n) {
    if (n > 1) {
        fizzbuzz(n - 1);
    }

    if (n % 15 == 0) {
        print_fizzbuzz();
    } else if (n % 3 == 0) {
        print_fizz();
    } else if (n % 5 == 0) {
        print_buzz();
    } else {
        print_int(n);
    }
    putchar(10);
}

int main(void) {
    fizzbuzz(50);
    return 0;
}
