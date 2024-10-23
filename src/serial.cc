/* Copyright 2012 William Woodall and John Harrison */
#include <algorithm>

#if !defined(_WIN32) && !defined(__OpenBSD__) && !defined(__FreeBSD__)
#include <alloca.h>
#endif

#include "serial/serial.h"

#ifdef _WIN32
#include "serial/impl/win.h"
#else
#include "serial/impl/unix.h"
#endif

using std::invalid_argument;
using std::min;
using std::numeric_limits;
using std::size_t;
using std::string;
using std::vector;

using serial::bytesize_t;
using serial::flowcontrol_t;
using serial::IOException;
using serial::parity_t;
using serial::Serial;
using serial::SerialException;
using serial::stopbits_t;

class Serial::ScopedReadLock {
public:
    ScopedReadLock(SerialImpl* pimpl)
        : pimpl_(pimpl)
    {
        this->pimpl_->readLock();
    }
    ~ScopedReadLock()
    {
        this->pimpl_->readUnlock();
    }

private:
    // Disable copy constructors
    ScopedReadLock(const ScopedReadLock&);
    const ScopedReadLock& operator=(ScopedReadLock);

    SerialImpl* pimpl_;
};

class Serial::ScopedWriteLock {
public:
    ScopedWriteLock(SerialImpl* pimpl)
        : pimpl_(pimpl)
    {
        this->pimpl_->writeLock();
    }
    ~ScopedWriteLock()
    {
        this->pimpl_->writeUnlock();
    }

private:
    // Disable copy constructors
    ScopedWriteLock(const ScopedWriteLock&);
    const ScopedWriteLock& operator=(ScopedWriteLock);
    SerialImpl* pimpl_;
};

size_t
Serial::read_(uint8_t* buffer, size_t size)
{
    return this->pimpl_->read(buffer, size);
}

size_t
Serial::read(uint8_t* buffer, size_t size)
{
    ScopedReadLock lock(this->pimpl_);
    return this->pimpl_->read(buffer, size);
}

size_t
Serial::read(std::vector<uint8_t>& buffer, size_t size)
{
    ScopedReadLock lock(this->pimpl_);
    uint8_t* buffer_ = new uint8_t[size];
    size_t bytes_read = 0;

    try {
        bytes_read = this->pimpl_->read(buffer_, size);
    }
    catch (const std::exception&) {
        delete[] buffer_;
        throw;
    }

    buffer.insert(buffer.end(), buffer_, buffer_ + bytes_read);
    delete[] buffer_;
    return bytes_read;
}

size_t
Serial::read(std::string& buffer, size_t size)
{
    ScopedReadLock lock(this->pimpl_);
    uint8_t* buffer_ = new uint8_t[size];
    size_t bytes_read = 0;
    try {
        bytes_read = this->pimpl_->read(buffer_, size);
    }
    catch (const std::exception&) {
        delete[] buffer_;
        throw;
    }
    buffer.append(reinterpret_cast<const char*>(buffer_), bytes_read);
    delete[] buffer_;
    return bytes_read;
}

string
Serial::read(size_t size)
{
    std::string buffer;
    this->read(buffer, size);
    return buffer;
}

size_t
Serial::readline(string& buffer, size_t size, string eol)
{
    ScopedReadLock lock(this->pimpl_);
    size_t eol_len = eol.length();
    uint8_t* buffer_ = static_cast<uint8_t*>(alloca(size * sizeof(uint8_t)));
    size_t read_so_far = 0;
    while (true) {
        size_t bytes_read = this->read_(buffer_ + read_so_far, 1);
        read_so_far += bytes_read;
        if (bytes_read == 0) {
            break; // Timeout occured on reading 1 byte
        }
        if (string(reinterpret_cast<const char*>(buffer_ + read_so_far - eol_len), eol_len) == eol) {
            break; // EOL found
        }
        if (read_so_far == size) {
            break; // Reached the maximum read length
        }
    }
    buffer.append(reinterpret_cast<const char*>(buffer_), read_so_far);
    return read_so_far;
}

string
Serial::readline(size_t size, string eol)
{
    std::string buffer;
    this->readline(buffer, size, eol);
    return buffer;
}

vector<string>
Serial::readlines(size_t size, string eol)
{
    ScopedReadLock lock(this->pimpl_);
    std::vector<std::string> lines;
    size_t eol_len = eol.length();
    uint8_t* buffer_ = static_cast<uint8_t*>(alloca(size * sizeof(uint8_t)));
    size_t read_so_far = 0;
    size_t start_of_line = 0;
    while (read_so_far < size) {
        size_t bytes_read = this->read_(buffer_ + read_so_far, 1);
        read_so_far += bytes_read;
        if (bytes_read == 0) {
            if (start_of_line != read_so_far) {
                lines.push_back(
                    string(reinterpret_cast<const char*>(buffer_ + start_of_line),
                        read_so_far - start_of_line));
            }
            break; // Timeout occured on reading 1 byte
        }
        if (string(reinterpret_cast<const char*>(buffer_ + read_so_far - eol_len), eol_len) == eol) {
            // EOL found
            lines.push_back(
                string(reinterpret_cast<const char*>(buffer_ + start_of_line),
                    read_so_far - start_of_line));
            start_of_line = read_so_far;
        }
        if (read_so_far == size) {
            if (start_of_line != read_so_far) {
                lines.push_back(
                    string(reinterpret_cast<const char*>(buffer_ + start_of_line),
                        read_so_far - start_of_line));
            }
            break; // Reached the maximum read length
        }
    }
    return lines;
}

size_t
Serial::write(const string& data)
{
    ScopedWriteLock lock(this->pimpl_);
    return this->write_(reinterpret_cast<const uint8_t*>(data.c_str()),
        data.length());
}

size_t
Serial::write(const std::vector<uint8_t>& data)
{
    ScopedWriteLock lock(this->pimpl_);
    return this->write_(&data[0], data.size());
}

size_t
Serial::write(const uint8_t* data, size_t size)
{
    ScopedWriteLock lock(this->pimpl_);
    return this->write_(data, size);
}

size_t
Serial::write_(const uint8_t* data, size_t length)
{
    return pimpl_->write(data, length);
}
