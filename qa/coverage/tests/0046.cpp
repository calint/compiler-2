#include <iostream>
int main() {
    int a = 4;
    int b = 2;
    int c = 0;
    int d = 1;
    int e = 5;

    c = -(-((a + b) + d) - (-e));
    std::cout << c << '\n';

    c = (-(a + d) % b) * (e / a);
    std::cout << c << '\n';

    c = e * d + a * b - a * d;
    std::cout << c << '\n';

    c = -(a + -b);
    std::cout << c << '\n';

    c = a - e * b / a + b % (-a);
    std::cout << c << "\n";

    c = b % (-a);
    std::cout << c << '\n';

    c = (a % b + e / b) * (a / d);
    std::cout << c << '\n';
}
