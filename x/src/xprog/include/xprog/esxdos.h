#ifndef XPROG_ESXDOS_H
#define XPROG_ESXDOS_H

#include <cstdint>
#include <string>
#include <vector>

namespace xprog {

/* Build a partitioned 16 MiB IDE image with one FAT16 root file. */
std::vector<std::uint8_t> build_esxdos_disk(
    const std::vector<std::uint8_t>& file,
    const std::string& name);

} // namespace xprog

#endif
