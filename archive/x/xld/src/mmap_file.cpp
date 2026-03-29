// mmap_file.cpp
#include "mmap_file.h"

mmap_file::mmap_file(const char* filename)
    : file_descriptor_(-1), file_size_(0), mapped_data_(nullptr) {
    // Open the file
    file_descriptor_ = open(filename, O_RDONLY);
    if (file_descriptor_ == -1) {
        throw std::ios_base::failure("Error opening file: " + std::string(strerror(errno)));
    }

    // Get the file size
    file_size_ = lseek(file_descriptor_, 0, SEEK_END);
    lseek(file_descriptor_, 0, SEEK_SET); // Reset file position

    // Memory-map the file
    mapped_data_ = static_cast<const char*>(mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, file_descriptor_, 0));
    if (mapped_data_ == MAP_FAILED) {
        close(file_descriptor_);
        throw std::ios_base::failure("Error mapping file: " + std::string(strerror(errno)));
    }
}

mmap_file::~mmap_file() {
    if (mapped_data_ && mapped_data_ != MAP_FAILED) {
        munmap(const_cast<char*>(mapped_data_), file_size_); // Unmap memory
    }
    if (file_descriptor_ != -1) {
        close(file_descriptor_); // Close file
    }
}

const char* mmap_file::data() const {
    return mapped_data_;
}

std::size_t mmap_file::size() const {
    return file_size_;
}

bool mmap_file::is_open() const {
    return mapped_data_ != nullptr;
}
