#include <iostream>
int main() {

    int x = 5;
    int y = 10;
    int z = 15;
    int w = 20;
    int a = 3;
    int b = 7;
    bool flag1 = true;
    bool flag2 = false;
    bool flag3 = true;

    bool test6 =
        (((x & y & z) == 0) and ((w | a | b) == 31)) or not((x ^ y ^ z) == 0);
    std::cout << "test6: " << test6 << "\n";

    bool dummy = flag1 and flag2 and flag3;
    std::cout << "dummy: " << dummy << "\n";
}
