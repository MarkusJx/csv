#include <iostream>
#include "csv.hpp"

#include <fstream>

int main() {
    markusjx::csv c;
    c << "a" << "b" << "c" << markusjx::csv::endl;
    c << 1 << 2 << 3 << true;

    std::ofstream out("abc.csv");
    out << c;

    c = markusjx::csv::parse(c.to_string());
    std::wcout << c;
    return 0;
}
