/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2022 Benjamin Gerard
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "dumpsid.h"
#include "dumpsid-emu.h"
#include "sidcxx11.h"

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cinttypes>

#include <string>
#include <memory>
#include <sstream>
#include <algorithm>
#include <new>

#include <fcntl.h>
#include <unistd.h>

#define MAXSIDS 8

DumpSIDBuilder::DumpSIDBuilder(const char * const label,
			       const char * fn, int fd) :
    sidbuilder(label),
    m_fn(), m_fd(-1)
{
    if (!m_status)
        return;

#if DEBUG >= 10
    ::printf("DumpSIDBuilder(<%s>,<%s>,<%i>)\n", name(), !fn?"(nil)":fn,fd);
#endif

    if (fd != -1) {
        m_fd = fd;
        if (fn != NULL)
            m_fn.assign(fn);
        else {
            char tmp[64];
            int len = ::snprintf(tmp, sizeof(tmp), ">&%i", fd);
            m_fn.assign(tmp,len);
        }
    }
    else {
        m_fn.assign(fn);
        if(m_fd = ::open(fn, O_WRONLY|O_CREAT|O_TRUNC, 0666), m_fd == -1)
            setErrno("open");
    }
    assert ( (m_fd != -1) == m_status );
#if DEBUG >= 10
    ::printf("DumpSIDBuilder(<%s>,<%s>,<%i>) -> %s\n",
	     name(), m_fn.c_str(), m_fd, m_status?"OK":"ERR");
#endif
}

#include <iostream> // $$$ DELME

DumpSIDBuilder::~DumpSIDBuilder()
{   // Remove all SID emulations
#if defined(DEBUG) && DEBUG >= 10
    ::printf("DumpSIDBuilder::~DumpSIDBuilder()\n");
#endif
    if (m_fd != -1)
        ::close(m_fd);
}

void DumpSIDBuilder::setError(const char * str)
{
    m_status = false;
    m_errorBuffer
        . assign(name())
        . append(": ")
        . append(str?str:"(nil)")
        . append(" -- ")
        . append(m_fn)
        ;
}

void DumpSIDBuilder::setErrno(int errnum, const char * fct)
{
    std::string buffer;
    buffer
        .assign("(")
        . append(fct)
        . append(") ")
        . append(::strerror(errnum))
        ;
    setError(buffer.c_str());
}

void DumpSIDBuilder::setErrno(const char * fct)
{
    if (errno)
        setErrno(errno, fct);
}

void DumpSIDBuilder::flush()
{
    if (m_fd != -1 && ::fsync(m_fd) == -1 && errno != EINVAL)
        // EINVAL occurs for socket, pipes ...
        setErrno("fsync");
}

void DumpSIDBuilder::dumpStr(const char * str, int len)
{
    int n = ::write(m_fd, str, len);
    if (n == -1)
        setErrno("write");
    else if (len != n)
        setError("truncated <write>");
}

// Create a new sid emulation.
unsigned int DumpSIDBuilder::create(unsigned int sids)
{
    unsigned int count;

    if (!m_status)
        return 0;

    sids = std::min(sids,unsigned(MAXSIDS));

    for (count=0; count<sids; ++count)
    {
	try
	{
	    std::unique_ptr<libsidplayfp::DumpSID>
                sid(new libsidplayfp::DumpSID(this, count, m_fd, m_fn.c_str()));

	    // SID init failed?
	    if (!sid->getStatus())
	    {
		m_errorBuffer = sid->error();
                break;
	    }
	    sidobjs.insert(sid.release());
	}
	// Memory alloc failed?
	catch (std::bad_alloc const &)
	{
	    m_errorBuffer.assign(name()).append(": ").append(::strerror(ENOMEM));
	    break;
	}
    }
#if DEBUG == 10
    ::printf("DumpSIDBuilder::create(%u) -> %u\n", sids, count);
#endif
    m_status = count > 0;
    return count;
}

const char *DumpSIDBuilder::credits() const
{
    return libsidplayfp::DumpSID::getCredits();
}

void DumpSIDBuilder::filter(bool enable)
{
#if DEBUG == 10
    ::printf("DumpSIDBuilder::filter(%d)\n", int(enable));
#endif
    std::for_each(sidobjs.begin(), sidobjs.end(),
                  applyParameter<libsidplayfp::DumpSID,
                  bool>(&libsidplayfp::DumpSID::filter, enable));
}

unsigned int DumpSIDBuilder::availDevices() const
{
    return MAXSIDS;
}

void DumpSIDBuilder::setInfo(const char * file, const char *name,
                             const char *author, int num)
{
    m_info.set = true;
    m_info.num = num;
    m_info.file.assign(file);
    m_info.title.assign(name);
    m_info.author.assign(author);

    if (m_fd) {
        std::stringstream s;
        s << "!SID-FILE: <" << file << '>';
        if (num > 0) s << " <" << num << '>';
        s << std::endl
          << "!SID-TITLE: <" << name << '>' << std::endl
          << "!SID-AUTHOR: <" << author << '>' << std::endl;
        dumpStr(s.str().c_str(), s.str().length());
    }
}
