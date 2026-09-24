// SPDX-FileCopyrightText: 2026 Fu Wenbo
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE (64 * 1024)

static void fail(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static void read_exact(int fd, unsigned char *buffer, size_t count, const char *name) {
    while (count != 0) {
        ssize_t result = read(fd, buffer, count);
        if (result < 0) {
            fail(name);
        }
        if (result == 0) {
            fprintf(stderr, "%s: unexpected end of file\n", name);
            exit(EXIT_FAILURE);
        }
        buffer += result;
        count -= (size_t)result;
    }
}

static void write_exact(int fd, const unsigned char *buffer, size_t count, const char *name) {
    while (count != 0) {
        ssize_t result = write(fd, buffer, count);
        if (result < 0) {
            fail(name);
        }
        if (result == 0) {
            fprintf(stderr, "%s: short write\n", name);
            exit(EXIT_FAILURE);
        }
        buffer += result;
        count -= (size_t)result;
    }
}

int main(int argc, char **argv) {
    struct stat source_stat;
    struct mtd_info_user mtd;
    unsigned char *write_buffer;
    unsigned char *verify_buffer;
    int source;
    int target;
    off_t offset;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <image> <mtd-device>\n", argv[0]);
        return EXIT_FAILURE;
    }

    source = open(argv[1], O_RDONLY);
    if (source < 0) {
        fail(argv[1]);
    }
    if (fstat(source, &source_stat) != 0 || source_stat.st_size <= 0) {
        fail("image size");
    }

    target = open(argv[2], O_RDWR | O_SYNC);
    if (target < 0) {
        fail(argv[2]);
    }
    if (ioctl(target, MEMGETINFO, &mtd) != 0) {
        fail("MEMGETINFO");
    }
    if ((uint64_t)source_stat.st_size > mtd.size) {
        fprintf(stderr, "image is larger than MTD partition\n");
        return EXIT_FAILURE;
    }

    printf("Erasing %u-byte %s partition...\n", mtd.size, argv[2]);
    for (offset = 0; offset < mtd.size; offset += mtd.erasesize) {
        struct erase_info_user erase = {
            .start = (uint32_t)offset,
            .length = mtd.erasesize,
        };
        if (ioctl(target, MEMERASE, &erase) != 0) {
            fail("MEMERASE");
        }
    }

    write_buffer = malloc(BUFFER_SIZE);
    verify_buffer = malloc(BUFFER_SIZE);
    if (write_buffer == NULL || verify_buffer == NULL) {
        fail("malloc");
    }

    printf("Writing %lld bytes...\n", (long long)source_stat.st_size);
    for (offset = 0; offset < source_stat.st_size;) {
        size_t chunk = (size_t)(source_stat.st_size - offset);
        if (chunk > BUFFER_SIZE) {
            chunk = BUFFER_SIZE;
        }
        read_exact(source, write_buffer, chunk, argv[1]);
        write_exact(target, write_buffer, chunk, argv[2]);
        offset += (off_t)chunk;
    }
    /* Raw MTD character devices commonly reject fsync after synchronous writes. */
    if (fsync(target) != 0 && errno != EINVAL) {
        fail("fsync");
    }

    if (lseek(source, 0, SEEK_SET) < 0 || lseek(target, 0, SEEK_SET) < 0) {
        fail("lseek");
    }
    printf("Verifying...\n");
    for (offset = 0; offset < source_stat.st_size;) {
        size_t chunk = (size_t)(source_stat.st_size - offset);
        if (chunk > BUFFER_SIZE) {
            chunk = BUFFER_SIZE;
        }
        read_exact(source, write_buffer, chunk, argv[1]);
        read_exact(target, verify_buffer, chunk, argv[2]);
        if (memcmp(write_buffer, verify_buffer, chunk) != 0) {
            fprintf(stderr, "verification failed at offset 0x%llx\n", (long long)offset);
            return EXIT_FAILURE;
        }
        offset += (off_t)chunk;
    }

    printf("Verified %lld bytes in %s\n", (long long)source_stat.st_size, argv[2]);
    return EXIT_SUCCESS;
}
