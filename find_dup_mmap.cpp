#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <filename>\n";
        return -1;
    }

    // open a file
    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
        handle_error("open");

    // get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1)
        handle_error("fstat");

    char * addr = (char *) mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED)
        handle_error("mmap");

    uint64_t count = 0;

    // implementation based on vector<bool> ~ 0.6 sec
    // vector<int> ~ 1.2 sec
    // with stdio readline ~ 0.45 sec
    // custom impl of bit vector ~ 0.45 sec
    // with mmap ~ 0.119 sec

    using bits_type = uint64_t;
    constexpr std::size_t cBits = sizeof(bits_type) * 8;
    constexpr std::size_t cContSize = 26 * 26 * 26 * 10 * 10 * 10;

    std::vector<bits_type> cont(cContSize / cBits + 1, 0);

    for (char * line = addr; line < addr + sb.st_size; ++line)
    {
        std::size_t pos = 0;

        pos += 26L * 26 * (*line++ - 'A');
        pos += 26L * (*line++ - 'A');
        pos += (*line++ - 'A');
        pos *= 1000;
        pos += 100L * (*line++ - '0');
        pos += 10L * (*line++ - '0');
        pos += (*line++ - '0');

        // try to eliminate conditions
        const std::size_t shift = pos % cBits;
        pos /= cBits;
        count += (cont[pos] >> shift) & 0x01;
        cont[pos] |= (1LL << shift);
    }

    std::cout << "found " << count << " duplicates\n";

    munmap(addr, sb.st_size);
    close(fd);

    return 0;
}

