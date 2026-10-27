#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define SW 160
#define SH 120
#define FW (SW * SH / 8)
#define BLOCK_MAX (SW * 2 * 2 * 2)

static int spi_fd, dc_fd;
static void die(const char *s)
{
    perror(s);
    exit(1);
}

static void dc(int v)
{
    char c = v ? '1' : '0';
    if (lseek(dc_fd, 0, SEEK_SET) < 0) die("gpio seek");
    if (write(dc_fd, &c, 1) != 1) die("gpio write");
}

static void send_bytes(const uint8_t *p, size_t n)
{
    while (n) {
        size_t chunk = n > 4096 ? 4096 : n;
        ssize_t r = write(spi_fd, p, chunk);

        if (r < 0) {
            if (errno == EINTR) continue;
            die("spi write");
        }
        if (r == 0) {
            fprintf(stderr, "SPI write returndd 0\n");
            exit(1);
        }

        p += r;
        n -= (size_t)r;
    }
}

static void command(uint8_t c, const uint8_t *data, size_t n)
{
    dc(0);
    send_bytes(&c, 1);
    dc(1);
    if (n) send_bytes(data, n);
}

static void set_window(int x0, int y0, int x1, int y1)
{
    uint8_t x[] = {
        (uint8_t)(x0 >> 8), (uint8_t)x0,
        (uint8_t)(x1 >> 8), (uint8_t)x1
    };
    uint8_t y[] = {
        (uint8_t)(y0 >> 8), (uint8_t)y0,
        (uint8_t)(y1 >> 8), (uint8_t)y1
    };

    command(0x2A, x, 4);
    command(0x2B, y, 4);
    command(0x2C, NULL, 0);
}

static int pixel(const uint8_t *f, int x, int y)
{
    return (f[y * (SW / 8) + x / 8] >> (7 - x % 8)) & 1;
}

static void draw_frame(const uint8_t *src, uint8_t *prev,
                       uint8_t *buf, int first)
{
    for (int y = 0; y < SH; y++) {
        int minx = SW, maxx = -1;
        for (int x = 0; x < SW; x++) {
            if (first || pixel(src, x, y) != pixel(prev, x, y)) {
                if (x < minx) minx = x;
                if (x > maxx) maxx = x;
            }
        }

        if (maxx < minx)
            continue;

        int width = maxx - minx + 1;
        size_t row_bytes = (size_t)width * 4;
        size_t k = 0;
        for (int x = minx; x <= maxx; x++) {
            uint16_t c = pixel(src, x, y) ? 0xFFFF : 0x0000;

            buf[k++] = c >> 8;
            buf[k++] = c & 255;
            buf[k++] = c >> 8;
            buf[k++] = c & 255;
        }
        memcpy(buf + row_bytes, buf, row_bytes);
        set_window(minx * 2, y * 2, maxx * 2 + 1, y * 2 + 1);
        send_bytes(buf, row_bytes * 2);
    }memcpy(prev, src, FW);
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "/tmp/badapple.baf";
    FILE *f = fopen(path, "rb");
    if (!f) die("open video");
    spi_fd = open("/dev/spidev1.0", O_WRONLY);
    if (spi_fd < 0) die("open spidev");
    dc_fd = open("/sys/class/gpio/gpio12/value", O_WRONLY);
    if (dc_fd < 0) die("open GPIO12");
    uint8_t src[FW];
    uint8_t prev[FW];
    uint8_t *buf = malloc(BLOCK_MAX);
    if (!buf) die("malloc");
    memset(prev, 0, sizeof(prev));
    unsigned long frames = 0;
    int first = 1;
    struct timespec begin, now, deadline;
    clock_gettime(CLOCK_MONOTONIC, &begin);
    deadline = begin;

    while (fread(src, 1, FW, f) == FW) {
        draw_frame(src, prev, buf, first);
        first = 0;
        frames++;

        deadline.tv_nsec += 100000000L;
        while (deadline.tv_nsec >= 1000000000L) {
            deadline.tv_sec++;
            deadline.tv_nsec -= 1000000000L;
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec < deadline.tv_sec ||
            (now.tv_sec == deadline.tv_sec &&
             now.tv_nsec < deadline.tv_nsec)) {
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                            &deadline, NULL);
        }
        if (frames % 20 == 0) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            double elapsed = now.tv_sec - begin.tv_sec +
                (now.tv_nsec - begin.tv_nsec) / 1e9;
            fprintf(stderr, "%lu кадров, %.2f FPS\n",
                    frames, frames / elapsed);
        }
    }

    fprintf(stderr, "Конец. %lu кадров\n", frames);
    free(buf);
    fclose(f);
    close(dc_fd);
    close(spi_fd);
    return 0;
}
