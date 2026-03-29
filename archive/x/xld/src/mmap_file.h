// mmap_file.h
#ifndef MMAP_FILE_H
#define MMAP_FILE_H

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <ios>

class mmap_file {
public:
    mmap_file(const char* filename);
    ~mmap_file();

    const char* data() const;
    std::size_t size() const;
    bool is_open() const;

private:
    int file_descriptor_;
    std::size_t file_size_;
    const char* mapped_data_;
};

#endif // MMAP_FILE_H
