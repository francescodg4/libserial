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

