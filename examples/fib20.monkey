let fibonacci = fn(x) {
    if (x < 2) {
        return x;
    }

    return fibonacci(x - 1) + fibonacci(x - 2);
}

puts("20th fibonacci number is: ", fibonacci(20))