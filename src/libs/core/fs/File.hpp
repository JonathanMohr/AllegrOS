#pragma once
#include <stddef.h>
#include <stdint.h>

enum class SeekPos {
    Set,
    Current,
    End,
};

class File {
public:
    virtual ~File() { }

    virtual size_t Read(uint8_t* data, size_t size) = 0;
    virtual size_t Write(const uint8_t* data, size_t size) = 0;
    virtual void Seek(SeekPos pos, int rel) = 0;
    virtual size_t Size() = 0;
};