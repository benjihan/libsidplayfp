/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012 Leando Nini <drfiemost@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PSID_H
#define PSID_H

#include "SidTuneBase.h"

class PSID : public SidTuneBase
{
 private:
    //uint_least32_t fileOffset;

    char m_md5[SidTune::MD5_LENGTH+1];

 private:
    bool resolveAddrs(const uint_least8_t *c64data);
    bool checkRelocInfo(void);

    void tryLoad(Buffer_sidtt<const uint_least8_t>& dataBuf);

 protected:
    PSID() {}

 public:
    virtual ~PSID() {}

    static SidTuneBase* load(Buffer_sidtt<const uint_least8_t>& dataBuf);

    //bool placeSidTuneInC64mem(uint_least8_t* c64buf);

    virtual const char *createMD5(char *md5);
};

#endif // PSID_H