#include <iostream>

#include <unistd.h>

int main()
{
    fork(); fork(); fork();
    std::cout << "hi!\n";
    return 0;
}
