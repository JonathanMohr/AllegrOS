#include "file.h"

#include <syscall/close.h>
#include <syscall/open.h>
#include <syscall/read.h>
#include <syscall/write.h>

#include <string.h>
#include <stddef.h>

#define FILE_STREAMS 512
FILE streams[FILE_STREAMS];

FILE* stdin;
FILE* stdout;
FILE* stderr;
FILE* stddebug;

// INIT

void finit()
{
    stdin = &streams[0];
    stdin->fd = 0;
    stdin->buf_pos = 0;
    stdin->buf_end = 0;

    stdout = &streams[1];
    stdout->fd = 1;

    stderr = &streams[2];
    stdin->fd = 2;

    stddebug = &streams[3];
    stddebug->fd = 3;

    for (int i = 4; i < FILE_STREAMS; i++)
    {
        streams[i].fd = -1;
    }
}


// WRITE

void fputc(char c, FILE* stream)
{
    if (!stream)
        return;

    //TODO: check result
    write(stream->fd, &c, 1);
}

void fputs(const char* str, FILE* stream)
{
    if (!stream)
        return;

    //TODO: check result
    write(stream->fd, str, (uint32_t)strlen(str));
}


// READ

#define EOF -1

int fgetc(FILE* stream)
{
    if (!stream)
        return EOF;

    if (stream->buf_pos >= stream->buf_end)
    {
        int32_t n = read(stream->fd, stream->buffer, BUFFER_SIZE);
        if (n <= 0)
            return EOF; // EOF or error

        stream->buf_pos = 0;
        stream->buf_end = n;
    }

    return (int)(unsigned char) stream->buffer[stream->buf_pos++];
}

char* fgets(char* buf, int size, FILE* stream)
{
    if (size <= 0 || buf == NULL)
        return NULL;

    int i = 0;
    while (i < size - 1)
    {
        if (stream->buf_pos >= stream->buf_end)
        {
            int32_t n = read(stream->fd, stream->buffer, BUFFER_SIZE);
            if (n <= 0)
            {
                // EOF or error
                if (i == 0)
                    return NULL;    // nothing read
                break;              // read something -> cancel
            }
            stream->buf_pos = 0;
            stream->buf_end = n;
        }

        char c = stream->buffer[stream->buf_pos++];
        buf[i++] = c;
        if (c == '\n')
            break;
    }

    buf[i] = '\0';
    return buf;
}

int32_t fread(void* ptr, int32_t size, int32_t count, FILE* stream)
{
    if (!stream || !ptr || size == 0 || count == 0)
        return 0;

    int32_t total_bytes = size * count;
    int32_t bytes_read = 0;
    unsigned char* buffer = (unsigned char*) ptr;

    while (bytes_read < total_bytes)
    {
        if (stream->buf_pos >= stream->buf_end)
        {
            // Puffer neu füllen
            int64_t n = read(stream->fd, stream->buffer, BUFFER_SIZE);
            if (n <= 0) // EOF oder Fehler
                break;

            stream->buf_pos = 0;
            stream->buf_end = n;
        }

        // Wie viele Bytes können wir noch aus dem Puffer lesen?
        int32_t available = stream->buf_end - stream->buf_pos;
        int32_t to_copy = total_bytes - bytes_read;
        if (to_copy > available)
            to_copy = available;

        // Kopiere die Bytes in den Zielpuffer
        for (int32_t i = 0; i < to_copy; i++)
        {
            buffer[bytes_read + i] = stream->buffer[stream->buf_pos + i];
        }

        stream->buf_pos += to_copy;
        bytes_read += to_copy;
    }

    // Anzahl gelesener Elemente zurückgeben (Bytes / size)
    return bytes_read / size;
}


// OPEN

FILE* fopen(const char* path, const char* mode)
{
    int flags;
    uint16_t mode_flag = 0;

    if (!path || !mode)
    {
        return NULL;
    }

    if (strcmp(mode, "r") == 0) {
        flags = 0; // TODO
        mode_flag = 0; // kein Modus nötig
    }
    else if (strcmp(mode, "w") == 0) {
        flags = 0;  // TODO
        mode_flag = 0644; // typische Rechte
    }
    else if (strcmp(mode, "a") == 0) {
        flags = 0;  // TODO
        mode_flag = 0644;
    }
    else {
        return NULL; // nicht unterstützt
    }

    int32_t fd = open(path, flags, mode_flag);
    if (fd < 0)
        return NULL;

    FILE* f = NULL;
    for (int i = 0; i < FILE_STREAMS; i++) {
        if (streams[i].fd == -1)
        {
            f = &streams[i];
            break;
        }
    }
    if (!f)
    {
        close(fd);
        return NULL;
    }

    f->fd = fd;
    f->buf_pos = 0;
    f->buf_end = 0;

    return f;
}


// CLOSE

int fclose(FILE* stream)
{
    if (!stream)
        return -1;

    int result = close(stream->fd);
    //TODO: add free
    stream->fd = -1;
    return result;
}