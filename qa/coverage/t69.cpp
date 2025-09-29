
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
}
