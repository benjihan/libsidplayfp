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

#ifndef	 DUMPSID_H
#define	 DUMPSID_H

#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/siddefs.h"

namespace libsidplayfp { class DumpSID; }

/**
 * DumpSID Builder Class
 */
class SID_EXTERN DumpSIDBuilder : public sidbuilder
{
    friend class libsidplayfp::DumpSID;

private:
    enum { MAXSIDS = 8 };	      // 32 registers for 8 sids => 1 byte address

    bool m_opened;                      // true if open() by me
    int m_fd;				// dump file descriptor
    std::string m_fn;			// dump filename

    void setError(const char *);
    void setErrno(int errnum, const char * fct);
    void setErrno(const char * fct);
    void dumpStr(const char * str, int len);

    struct info {
      bool set;
      int num;
      std::string file, title, author;
      info() : set(false), num(0) {};
    } m_info;

public:
    DumpSIDBuilder(const char * const name, const char * fn, int fd=-1);
    ~DumpSIDBuilder();

    // Interface sidbuilder
    unsigned int availDevices() const;
    unsigned int create(unsigned int sids);
    const char *credits() const;
    void filter(bool enable);

    void setInfo(const char *file, const char *name, const char *auth, int song=-1);
    void flush();
};

#endif // DUMPSID_H
