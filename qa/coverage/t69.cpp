#include <iostream>

int main() {
    int x = 1;
    int y = 3;
    int z = 4;
    int t1 = 5;
    int t2 = 5;
    int t3 = 10;
    int t4 = 2;
    bool t5 = false;

    bool r = (x + y <= z) and (not(z == 4) or (x == 1 and y == 3));
    bool aa = ((t1 == t2 and t3 > t1 * t4) or t5) and not(t1 != t2 or t3 < 10);
    bool bb = not(t5 or (t1 + t2 > t3) or (t3 / t4 == t1)) and t1 == 5;
    bool cc = not(t1 < t2 and t3 > t4 and t5) or (x == 1 and not(y == z));
    bool dd = ((t1 == t2) and (t3 == t1 * t4)) or (t1 != 5 and t2 != 5);
    bool ee = not((t1 > 0 and t2 < 10) and not(t3 == 10)) and (t4 == 2 or t5);
    bool ff = not(t5) and ((t1 == t2 or t3 != t4) and not(t4 > t3 / 2));
    bool gg = (t1 == 5 and (t2 != t3 and t4 < t3)) or (t3 < t1 + t2 and t5);
    bool hh = ((t1 == t2) and (t4 != t1 and t5 == false)) or (t1 + 1 == 6);
    bool ii = not(t1 == 5 and t2 == 5 and t3 == 10 and t4 == 2 and t5 == false);
    bool jj = ((t1 + t2 == t3) or (t3 / t4 == t1)) and not(t1 < t5 or t2 < t5);
    bool kk = not(t1 == t2 and t3 / t4 == t1);

    std::cout << " r: " << r << "\n";
    std::cout << "aa: " << aa << "\n";
    std::cout << "bb: " << bb << "\n";
    std::cout << "cc: " << cc << "\n";
    std::cout << "dd: " << dd << "\n";
    std::cout << "ee: " << ee << "\n";
    std::cout << "ff: " << ff << "\n";
    std::cout << "gg: " << gg << "\n";
    std::cout << "hh: " << hh << "\n";
    std::cout << "ii: " << ii << "\n";
    std::cout << "jj: " << jj << "\n";
    std::cout << "kk: " << kk << "\n";

    bool h1 =
        not(not(t1 * 2 == t3) or (z * 2 > t3 + t4)) and (x < t4 and y > t1);
    bool h2 = ((t1 == 5 and t2 != 6) or (t3 > 100)) and not(z - y == x or t5);
    bool h3 = not(x < y and y < z and z > 0) or
              ((t1 == t2) and (t3 == 10) and not(t4 * t1 > 10));
    bool h4 = ((t1 + t2 + t4) and (t3 + 2) and (x == 1)) and
              not(t5 or (y > t1) or (t4 < 2));
    bool h5 = not((x == z - y) and (t3 / t4 == t1)) or
              (t1 + t2 > t3 and t3 - t1 == t2);
    bool h6 = not(t5 == true and x == 1) or
              ((t1 * t4 <= t3 and t3 == 10) or not(z * x != 4 or y == 3));
    bool h7 = ((t1 == t2 or t3 == t4) and not(z > x and y < z)) and
              ((t5 or x != 1) or (t1 + t4 == 7));
    bool h8 = not(t3 / t4 == t1) and
              ((t1 == t2 and z != 4) or not(x == 1 and y == 3 and z == 4));
    bool h9 = not((t1 + t2 + t3 + t4) > 20 and not t5) and
              (t3 / 5 > 1 or t4 / 1 == 2);
    bool h10 = ((t1 * t4 > t3 and t3 / t4 == t1) or (t1 == t2 and t3 == 10)) and
               not(t5 and t4 == 2);

    std::cout << "h1: " << h1 << "\n";
    std::cout << "h2: " << h2 << "\n";
    std::cout << "h3: " << h3 << "\n";
    std::cout << "h4: " << h4 << "\n";
    std::cout << "h5: " << h5 << "\n";
    std::cout << "h6: " << h6 << "\n";
    std::cout << "h7: " << h7 << "\n";
    std::cout << "h8: " << h8 << "\n";
    std::cout << "h9: " << h9 << "\n";
    std::cout << "h10: " << h10 << "\n";
}
