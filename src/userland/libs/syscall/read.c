#include "read.h"
#include "syscall.h"
#include <stddef.h>

int32_t read(int fd, void* buf, uint32_t count)
{
    return (int32_t)syscall(0x3, fd, (uint32_t)buf, count, 0, 0);
}


#define BUFFER_SIZE 0x1000

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
    uint32_t buf_pos;
    uint32_t buf_end;
} BufferedFile;

static BufferedFile buffered_files[256] = {0};

static BufferedFile* get_buffered_file(int fd)
{
    if (fd < 0 || fd >= 256)
        return NULL;

    BufferedFile* bf = &buffered_files[fd];
    if (bf->fd != fd) {
        bf->fd = fd;
        bf->buf_pos = 0;
        bf->buf_end = 0;
    }
    return bf;
}

char* fgets(char* buf, int size, int fd)
{
    if (size <= 0 || buf == NULL)
        return NULL;

    BufferedFile* bf = get_buffered_file(fd);
    if (!bf)
        return NULL;

    int i = 0;
    while (i < size - 1) {
        if (bf->buf_pos >= bf->buf_end) {
            int32_t n = read(fd, bf->buffer, BUFFER_SIZE);
            if (n <= 0) {
                // EOF oder Fehler
                if (i == 0)
                    return NULL;  // nichts gelesen
                break;          // schon was gelesen -> abbrechen
            }
            bf->buf_pos = 0;
            bf->buf_end = n;
        }

        char c = bf->buffer[bf->buf_pos++];
        buf[i++] = c;
        if (c == '\n')
            break;
    }

    buf[i] = '\0';
    return buf;
}