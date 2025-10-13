#include<map>
#include <iostream>
int main() {
    std::map<int, int> m;
    m[1] = 2;
    std::cout << m.size() << std::endl;
    auto m2 = std::move(m);
    std::cout << m.size() << std::endl;
    std::cout << m2.size() << std::endl;
}