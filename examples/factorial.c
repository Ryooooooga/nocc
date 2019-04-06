int factorial(int x) {
    if (x > 0) {
        return x * factorial(x - 1);
    } else {
        return 1;
    }
}

int main(void) {
    return factorial(5);
}
