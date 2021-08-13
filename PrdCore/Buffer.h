#pragma once

#include <Windows.h>
#include <deque>
#include <vector>
#include <stdint.h>

using namespace std;


class Buffer
{
public:
    Buffer(size_t bufferSize, bool blockOnFull);
    ~Buffer();

public:
    size_t Write(vector<uint8_t> const& writeBuffer);
    size_t Write(char const* writeBuffer, size_t writeCount);
    size_t Read(vector<uint8_t>& buffer, size_t readCount);
    void Clear();
    void SetBlockOnFull(bool blockOnFull);
    void SetSize(size_t size);
    void Pop(uint8_t& popValue);
    bool IsFull();

    deque<uint8_t> buffer_;
private:
    size_t bufferMax_;
    size_t readBufferExtension_;
    bool blockOnFull_;
    HANDLE dataInStoreEvent_;
    CRITICAL_SECTION mutex_;
};

