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

#ifndef DUMPSID_EMU_H
#define DUMPSID_EMU_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sidemu.h"
#include "Event.h"
#include "EventScheduler.h"
#include "sidplayfp/siddefs.h"
#include "sidcxx11.h"

#include <cstdarg>

class sidbuilder;
class DumpSIDBuilder;

namespace libsidplayfp
{

/***************************************************************************
 * DumpSID SID Specialisation
 ***************************************************************************/
class DumpSID final : public sidemu
{

private:
    // Dumpsid specific data
    static const char relfmt[];	   // default format (relative clock)
    static const char inifmt[];	   // default format (setup init)
    static const char jmpfmt[];    // default format (long clock)

    // SID registers
    // GB: should we make c64sid::lastpoke[] protected) instead ?
    uint8_t	   m_regs[0x20];
    int		   m_num;		// Number
    float	   m_sidfrq;		// SID clock frequence
    int		   m_model;		// SID model 6581 or 8520
    bool	   m_filter;		// filter active
    bool	   m_boost;		// digiboost enable
    uint8_t	   m_muted;		// muted voice bit mask
    event_clock_t  m_lastClk;		// last dumped clock

    // Dumping helpers
    void dumpStr(const char * str, int n);
    void dumpFmt(const char * fmt, ...);
    void dumpIni(unsigned clk, int adr, int vol, int sid, float frq);
    void dumpRel(unsigned clk, int adr, const char *dir, int val);
    void dumpReg(uint8_t addr, const char *dir, uint8_t data);

    inline uint8_t byteAddr(const uint8_t adr) const
    { return (adr&31) | (m_num<<5); }

public:
    DumpSID(sidbuilder *builder, int num);
    ~DumpSID();

    static const char* getCredits();

    // sidemuu:: interface
    void clock() override;
    void voice(unsigned int num, bool mute) override;
    void model(SidConfig::sid_model_t, bool digiboost) override;

    // sidemu:: override
    void sampling(float sidfrq, float, SidConfig::sampling_method_t, bool) override;

    // c64sid:: interface
    uint8_t read(uint_least8_t addr) override;
    void write(uint_least8_t addr, uint8_t data) override;
    void reset(uint8_t volume) override;

    bool getStatus() const { return m_status; }

    // DumpSID specific
    void flush();
    void filter(bool enable);

};

}

#endif // DUMPSID_EMU_H
