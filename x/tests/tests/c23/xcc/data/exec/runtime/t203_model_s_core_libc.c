#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdckdint.h>
#include <string.h>
#include <time.h>

static volatile unsigned long unsigned_value = 123456UL;
static volatile long signed_value = -123456L;
static volatile int checked_left = 300;
static volatile int checked_right = 200;

static int check_long_runtime(void) {
    unsigned long u = unsigned_value;
    long s = signed_value;

    if (u * 37UL != 4567872UL)
        return 1;
    if (u / 37UL != 3336UL || u % 37UL != 24UL)
        return 2;
    if (s / 37L != -3336L || s % 37L != -24L)
        return 3;
    return 0;
}

static int check_stdlib(void) {
    char formatted[16];
    char suffix;
    int first;
    int parsed;
    int second;
    int product;

    if (atoi("123") != 123 || atoi(" \t-42xyz") != -42 ||
        atoi("+17") != 17 || atoi("32767") != 32767 ||
        atoi("-32768") != (int)-32768L || atoi("xyz") != 0)
        return 1;

    srand(42U);
    first = rand();
    second = rand();
    if (first < 0 || first > RAND_MAX || second < 0 || second > RAND_MAX)
        return 2;
    srand(42U);
    if (rand() != first || rand() != second)
        return 3;

    if (!ckd_mul(&product, checked_left, checked_right))
        return 4;
    if (product != (int)60000L)
        return 5;
    if (ckd_mul(&product, 123, 45) || product != 5535)
        return 6;
    if (sscanf(" -123x", "%d%c", &parsed, &suffix) != 2 ||
        parsed != -123 || suffix != 'x')
        return 7;
    if (snprintf(formatted, sizeof(formatted), "%d/%x", -42, 0x2a) != 6 ||
        strcmp(formatted, "-42/2a") != 0)
        return 8;
    return 0;
}

static int check_file_positioning(void) {
    static const unsigned char payload[] = { 3, 7, 11, 19, 23 };
    const char *path = "s_core.tmp";
    fpos_t position;
    FILE *stream;
    int value;

    remove(path);
    stream = fopen(path, "wb");
    if (!stream)
        return 1;
    if (fwrite(payload, 1, sizeof(payload), stream) != sizeof(payload)) {
        fclose(stream);
        remove(path);
        return 2;
    }
    if (fclose(stream) != 0) {
        remove(path);
        return 3;
    }

    stream = fopen(path, "rb");
    if (!stream) {
        remove(path);
        return 4;
    }
    if (fseek(stream, 2L, SEEK_SET) != 0 || ftell(stream) != 2L) {
        fclose(stream);
        remove(path);
        return 5;
    }
    if (fgetpos(stream, &position) != 0) {
        fclose(stream);
        remove(path);
        return 6;
    }
    value = fgetc(stream);
    if (value != 11 || fsetpos(stream, &position) != 0 ||
        fgetc(stream) != value) {
        fclose(stream);
        remove(path);
        return 7;
    }
    rewind(stream);
    if (ftell(stream) != 0L || fgetc(stream) != 3) {
        fclose(stream);
        remove(path);
        return 8;
    }
    if (fclose(stream) != 0) {
        remove(path);
        return 9;
    }
    if (remove(path) != 0)
        return 10;
    return 0;
}

static int check_time(void) {
    time_t stamp = (time_t)-1;
    time_t day = 86400L;
    struct timespec resolution;
    struct timespec current;
    struct tm broken;
    struct tm local_broken;
    struct tm epoch;
    struct tm *static_broken;
    char calendar_text[26];
    char formatted_date[16];
    char *static_text;

    if (time(&stamp) != stamp || clock() < (clock_t)0)
        return 1;
    if (timespec_get(&current, TIME_UTC) != TIME_UTC)
        return 2;
    if (timespec_getres(&resolution, TIME_UTC) != TIME_UTC)
        return 3;
    if (gmtime_r(&day, &broken) != &broken)
        return 4;
    if (broken.tm_year != 70 || broken.tm_mon != 0 || broken.tm_mday != 2 ||
        broken.tm_hour != 0 || broken.tm_min != 0 || broken.tm_sec != 0 ||
        broken.tm_wday != 5 || broken.tm_yday != 1)
        return 5;
    static_broken = gmtime(&day);
    if (!static_broken || static_broken->tm_year != 70 ||
        static_broken->tm_mon != 0 || static_broken->tm_mday != 2)
        return 6;
    if (localtime_r(&day, &local_broken) != &local_broken ||
        local_broken.tm_year != 70 || local_broken.tm_mon != 0 ||
        local_broken.tm_mday != 2)
        return 7;
    static_broken = localtime(&day);
    if (!static_broken || static_broken->tm_year != 70 ||
        static_broken->tm_mon != 0 || static_broken->tm_mday != 2)
        return 8;

    if (asctime_r(&broken, calendar_text) != calendar_text ||
        strlen(calendar_text) != 25 || calendar_text[24] != '\n')
        return 9;
    static_text = asctime(&broken);
    if (!static_text || strcmp(static_text, calendar_text) != 0)
        return 10;
    if (ctime_r(&day, calendar_text) != calendar_text ||
        strlen(calendar_text) != 25 || calendar_text[24] != '\n')
        return 11;
    static_text = ctime(&day);
    if (!static_text || strcmp(static_text, calendar_text) != 0)
        return 12;
    if (strftime(formatted_date, sizeof(formatted_date), "%Y-%m-%d", &broken) != 10 ||
        strcmp(formatted_date, "1970-01-02") != 0)
        return 13;

    memset(&epoch, 0, sizeof(epoch));
    epoch.tm_year = 70;
    epoch.tm_mday = 1;
    if (timegm(&epoch) != 0L)
        return 14;
    memset(&epoch, 0, sizeof(epoch));
    epoch.tm_year = 70;
    epoch.tm_mday = 1;
    if (mktime(&epoch) != 0L)
        return 15;
    return 0;
}

int main(void) {
    int result;

    result = check_long_runtime();
    if (result)
        return 10 + result;
    result = check_stdlib();
    if (result)
        return 20 + result;
    result = check_file_positioning();
    if (result)
        return 30 + result;
    result = check_time();
    if (result)
        return 50 + result;
    return 0;
}
