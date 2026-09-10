/* Exercise disk calls whose application code and source objects stay in
 * ROM while esxDOS temporarily maps over that ROM.
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

volatile unsigned zx_disk_result;
volatile unsigned zx_disk_phase;

#define SEGMENT "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-"
static const char payload[] = SEGMENT SEGMENT SEGMENT SEGMENT
                              SEGMENT SEGMENT SEGMENT SEGMENT "!END";
static const char long_path[] = SEGMENT SEGMENT SEGMENT SEGMENT;
static const char first_path[] = "XROMIO.TMP";
static const char second_path[] = "XROM2.TMP";
static const char directory_path[] = "XROMDIR";

#define CHECK(n, condition) do { zx_disk_phase = (n); if (!(condition)) { \
    zx_disk_result = (n); for (;;) {} } } while (0)

int main(void)
{
    struct {
        unsigned char before;
        char bytes[sizeof payload];
        unsigned char after;
    } copy;
    struct stat status;
    char ram_path[16];
    char second_ram_path[16];
    char ram_text[] = "RAM";
    char cwd[256];
    int fd;

    CHECK(1, (uintptr_t)payload > 0 && (uintptr_t)payload < 0x4000);
    CHECK(2, (uintptr_t)first_path > 0 && (uintptr_t)first_path < 0x4000);
    CHECK(3, (uintptr_t)second_path < 0x4000
          && (uintptr_t)long_path < 0x4000);
    CHECK(4, open(NULL, O_RDONLY) == -1 && errno == EFAULT);
    CHECK(5, open("", O_RDONLY) == -1 && errno == ENOENT);
    CHECK(6, open(long_path, O_RDONLY) == -1 && errno == ENAMETOOLONG);
    unlink(first_path);
    unlink(second_path);
    unlink("XROM3.TMP");
    rmdir(directory_path);
    fd = open(first_path, O_CREAT | O_EXCL | O_RDWR);
    CHECK(7, fd >= 3);
    CHECK(8, write(fd, payload, sizeof payload) == sizeof payload);
    CHECK(9, lseek(fd, 0, SEEK_CUR) == sizeof payload);
    CHECK(10, write(fd, NULL, 0) == 0);
    CHECK(11, write(fd, NULL, 1) == -1 && errno == EFAULT);
    CHECK(12, write(fd, (const void *)0xffff, 2) == -1
           && errno == EFAULT);
    CHECK(13, read(fd, (void *)payload, 1) == -1 && errno == EFAULT);
    CHECK(14, lseek(fd, 0, SEEK_SET) == 0);
    copy.before = 0x35;
    copy.after = 0xa9;
    CHECK(15, read(fd, copy.bytes, sizeof copy.bytes) == sizeof payload);
    CHECK(16, memcmp(copy.bytes, payload, sizeof payload) == 0);
    CHECK(17, copy.before == 0x35 && copy.after == 0xa9);
    CHECK(18, close(fd) == 0);
    CHECK(19, stat(first_path, &status) == 0
           && status.st_size == sizeof payload);
    CHECK(20, stat(first_path, (struct stat *)payload) == -1
           && errno == EFAULT);
    CHECK(21, rename(first_path, second_path) == 0);
    strcpy(ram_path, "XROM3.TMP");
    CHECK(22, rename(second_path, ram_path) == 0);
    CHECK(23, rename(ram_path, first_path) == 0);
    strcpy(ram_path, first_path);
    strcpy(second_ram_path, second_path);
    CHECK(24, rename(ram_path, second_ram_path) == 0);
    CHECK(25, rename(second_ram_path, first_path) == 0);

    /* O_TRUNC without O_CREAT performs stat and open on the ROM path. */
    fd = open(first_path, O_RDWR | O_TRUNC);
    CHECK(26, fd >= 3);
    CHECK(27, fstat(fd, &status) == 0 && status.st_size == 0);
    CHECK(28, write(fd, payload, 127) == 127);
    CHECK(29, write(fd, payload + 127, 128) == 128);
    CHECK(30, write(fd, payload + 255, 129) == 129);
    CHECK(31, write(fd, payload + 384, sizeof payload - 384)
           == sizeof payload - 384);
    CHECK(32, write(fd, ram_text, 3) == 3);
    CHECK(33, close(fd) == 0);
    fd = open(first_path, O_WRONLY | O_APPEND);
    CHECK(34, fd >= 3 && lseek(fd, 0, SEEK_SET) == 0);
    CHECK(35, write(fd, payload, sizeof payload) == sizeof payload);
    CHECK(36, fsync(fd) == 0 && close(fd) == 0);
    CHECK(37, stat(first_path, &status) == 0
           && status.st_size == 2 * sizeof payload + 3);
    fd = open(ram_path, O_RDONLY);
    CHECK(38, fd >= 3);
    CHECK(39, read(fd, copy.bytes, sizeof payload) == sizeof payload
           && memcmp(copy.bytes, payload, sizeof payload) == 0);
    CHECK(40, read(fd, copy.bytes, 3) == 3
           && memcmp(copy.bytes, ram_text, 3) == 0);
    CHECK(41, read(fd, copy.bytes, sizeof payload) == sizeof payload
           && memcmp(copy.bytes, payload, sizeof payload) == 0);
    CHECK(42, read(fd, copy.bytes, 1) == 0 && close(fd) == 0);
    CHECK(43, mkdir(directory_path, 0777) == 0);
    CHECK(44, chdir(directory_path) == 0);
    CHECK(45, getcwd(cwd, sizeof cwd) == cwd
           && strstr(cwd, directory_path) != NULL);
    CHECK(46, getcwd((char *)payload, sizeof payload) == NULL
           && errno == EFAULT);
    CHECK(47, chdir("..") == 0 && rmdir(directory_path) == 0);
    CHECK(48, write(1, "\n", 1) == 1);
    CHECK(49, unlink(first_path) == 0);
    zx_disk_result = 0xa55a;
    return 0;
}
