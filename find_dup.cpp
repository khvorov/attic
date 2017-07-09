#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_set>

int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <filename>\n";
        return -1;
    }

    // open a file
    std::ifstream in(argv[1]);
    if (!in.is_open())
    {
        std::cerr << "failed to open a file: " << argv[1] << "\n";
        return -1;
    }

    uint64_t count = 0;

    // implementation based on associative containers:
    // set ~ 8 sec
    // unorderd_set ~ 4 sec

    std::set<std::string> s;
    //std::unordered_set<std::string> s;
    std::string line;
    while (std::getline(in, line))
    {
        if (!s.emplace(line).second)
            ++count;
    }

    std::cout << "found " << count << " duplicates\n";

    return 0;
}

