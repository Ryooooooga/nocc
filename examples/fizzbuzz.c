void print_fizzbuzz(void);
void print_fizz(void);
void print_buzz(void);
void print_int(int n);

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
}

int main(void) {
    fizzbuzz(50);
    return 0;
}
