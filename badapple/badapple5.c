#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#define SW 160
#define SH 120
#define FW (SW * SH / 8)
#define BLOCK_MAX (SW * 2 * 2 * 2)
#define LW 320
#define LH 240
#define DC_GPIO 12

#define SPI_IOC_WR_MODE          _IOW('k', 1, uint8_t)
#define SPI_IOC_WR_BITS_PER_WORD _IOW('k', 3, uint8_t)
#define SPI_IOC_WR_MAX_SPEED_HZ  _IOW('k', 4, uint32_t)

static int spi_fd;
static int dc_fd;

static void die(const char *msg)
{
    perror(msg);
    exit(1);
}

static void sysfs_write(const char *path, const char *val)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0) return;
    write(fd, val, strlen(val));
    close(fd);
}

static int gpio_out(int pin)
{
    char path[128];
    char buf[16];
    int fd;

    snprintf(buf, sizeof(buf), "%d", pin);
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", pin);

    if (access(path, F_OK) != 0) {
        sysfs_write("/sys/class/gpio/export", buf);
    }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    sysfs_write(path, "out");

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        die(path);
    }
    return fd;
}

static void dc(int state)
{
    char val = state ? '1' : '0';
    if (lseek(dc_fd, 0, SEEK_SET) < 0) die("gpio seek");
    if (write(dc_fd, &val, 1) != 1) die("gpio write");
}

static void send_bytes(const uint8_t *data, size_t len)
{
    while (len > 0) {
        size_t chunk = len;
        if (chunk > 4096) chunk = 4096;

        ssize_t res = write(spi_fd, data, chunk);
        if (res < 0) {
            if (errno == EINTR) continue;
            die("spi write");
        }
        if (res == 0) {
            fprintf(stderr, "SPI write error\n");
            exit(1);
        }
        data += res;
        len -= res;
    }
}

static void command(uint8_t cmd, const uint8_t *data, size_t len)
{
    dc(0);
    send_bytes(&cmd, 1);
    dc(1);
    if (len > 0) {
        send_bytes(data, len);
    }
}

static void set_window(int x0, int y0, int x1, int y1)
{
    uint8_t xb[4] = { x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF };
    uint8_t yb[4] = { y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF };

    command(0x2A, xb, 4);
    command(0x2B, yb, 4);
    command(0x2C, NULL, 0);
}

static void display_init(uint8_t madctl)
{
    static uint8_t zeros[4096];
    uint8_t colmod = 0x55;
    size_t left = LW * LH * 2;

    command(0x01, NULL, 0);
    usleep(150000);
    command(0x11, NULL, 0);
    usleep(150000);
    command(0x3A, &colmod, 1);
    command(0x36, &madctl, 1);

    set_window(0, 0, LW - 1, LH - 1);
    while (left > 0) {
        size_t sz = left;
        if (sz > sizeof(zeros)) sz = sizeof(zeros);
        send_bytes(zeros, sz);
        left -= sz;
    }

    command(0x29, NULL, 0);
    usleep(20000);
}

static int pixel(const uint8_t *frame, int x, int y)
{
    int index = y * (SW / 8) + (x / 8);
    int bit = 7 - (x % 8);
    return (frame[index] >> bit) & 1;
}

static void draw_frame(const uint8_t *src, uint8_t *prev, uint8_t *buf, int first)
{
    int y, x;

    for (y = 0; y < SH; y++) {
        int minx = SW;
        int maxx = -1;

        for (x = 0; x < SW; x++) {
            if (first || pixel(src, x, y) != pixel(prev, x, y)) {
                if (x < minx) minx = x;
                if (x > maxx) maxx = x;
            }
        }

        if (maxx < minx) continue;

        int width = maxx - minx + 1;
        size_t row_bytes = width * 4;
        size_t k = 0;

        for (x = minx; x <= maxx; x++) {
            uint16_t color = pixel(src, x, y) ? 0xFFFF : 0x0000;
            buf[k++] = color >> 8;
            buf[k++] = color & 0xFF;
            buf[k++] = color >> 8;
            buf[k++] = color & 0xFF;
        }

        memcpy(buf + row_bytes, buf, row_bytes);

        set_window(minx * 2, y * 2, maxx * 2 + 1, y * 2 + 1);
        send_bytes(buf, row_bytes * 2);
    }

    memcpy(prev, src, FW);
}

static double elapsed_since(const struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - start->tv_sec) + (now.tv_nsec - start->tv_nsec) / 1e9;
}

int main(int argc, char **argv)
{
    const char *path = "/tmp/badapple.baf";
    if (argc > 1) path = argv[1];

    double fps = 10.0;
    if (argc > 2) fps = atof(argv[2]);

    uint8_t madctl = 0x28;
    if (argc > 3) madctl = (uint8_t)strtoul(argv[3], NULL, 0);

    long period_ns = 0;
    if (fps > 0) period_ns = (long)(1e9 / fps);

    uint8_t mode = 0;
    uint8_t bits = 8;
    uint32_t speed = 20000000;

    FILE *f = fopen(path, "rb");
    if (!f) die("open video");

    spi_fd = open("/dev/spidev0.1", O_WRONLY);
    if (spi_fd < 0) die("open spidev");

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) die("mode");
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) die("bits");
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) die("speed");

    dc_fd = gpio_out(DC_GPIO);
    display_init(madctl);

    uint8_t src[FW];
    uint8_t prev[FW];
    uint8_t *buf = (uint8_t *)malloc(BLOCK_MAX);
    if (!buf) die("malloc");

    fprintf(stderr, "fps=%s madctl=0x%02X\n", fps > 0 ? "limit" : "no limit", madctl);
    if (fps > 0) {
        fprintf(stderr, "target fps: %.1f\n", fps);
    }

    while (1) {
        rewind(f);
        memset(prev, 0, sizeof(prev));

        unsigned long frames = 0;
        unsigned long late = 0;
        int first = 1;

        struct timespec begin, now, deadline;
        clock_gettime(CLOCK_MONOTONIC, &begin);
        deadline = begin;

        while (fread(src, 1, FW, f) == FW) {
            draw_frame(src, prev, buf, first);
            first = 0;
            frames++;

            if (period_ns > 0) {
                deadline.tv_nsec += period_ns;
                while (deadline.tv_nsec >= 1000000000L) {
                    deadline.tv_sec++;
                    deadline.tv_nsec -= 1000000000L;
                }

                clock_gettime(CLOCK_MONOTONIC, &now);
                if (now.tv_sec < deadline.tv_sec ||
                   (now.tv_sec == deadline.tv_sec && now.tv_nsec < deadline.tv_nsec)) {
                    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
                } else {
                    late++;
                }
            }

            if (frames % 20 == 0) {
                fprintf(stderr, "%lu frames, %.2f fps, late: %lu\n",
                        frames, frames / elapsed_since(&begin), late);
            }
        }

        double total = elapsed_since(&begin);
        fprintf(stderr, "Loop end: %lu frames in %.1f sec\n", frames, total);
    }

    free(buf);
    fclose(f);
    close(dc_fd);
    close(spi_fd);
    return 0;
}