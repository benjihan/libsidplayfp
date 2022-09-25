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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dumpsid-emu.h"
#include "dumpsid.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <sstream>
#include <string>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cinttypes>

// SID registers cheat sheet
//
//                 7|6|5|4|3|2|1|0
//                 -+-+-+-+-+-+-+-
// 00 V#1 FRQ(lo)  L L L L L L L L
// 01 V#1 FRQ(hi)  H H H H H H H H
// 02 V#1 PWM(lo)  L L L L L L L L
// 02 V#1 PWM(hi)  H H H H H H H H
// 04 V#1 CONTROL  N P S T D R Y G
//                 | | | | | | | |_ Gate 0:off/release 1:on/ads
//                 | | | | | | |___ Hard sYnc
//                 | | | | | |_____ Ring modulation
//                 | | | | |_______ Disable voice, reset noise generator.
//                 |_|_|_|_________ Waveforms {Noise,Pulse,Saw,Triangle}
//
// 05 V#1 AD       A A A A D D D D
// 06 V#1 SR       S S S S R R R R
// 07..0D V#2
// 0E..14 V#3
// 15 CUTOFF(lo)   . . . . . L L L
// 16 CUTOFF(hi)   H H H H H H H H
// 17 FILTER       R R R R X 3 2 1
//                 | | | | |_|_|_|_ Filter on (eXternal,#3,#2,#1}
//                 |_|_|_|_________ Resonance
// 18 MODES        3 H B L V V V V
//                 | | | | |_|_|_|_ Volume master
//                 | |_|_|_________ Filter{Highpass,Bandpass,Lowpass}
//                 |_______________ Disable voice #3
// 19 (RO)         Paddle X
// 1A (RO)         Paddle Y
// 1B (RO)         Voice #3 Waveform output
// 1C (RO)         Voice #3 ADSR output

namespace libsidplayfp
{

// const unsigned int DumpSID::voices = DUMPSID_VOICES;

const char DumpSID::inifmt[] = "%08X %02X %02X %04x %.4f\n";
const char DumpSID::relfmt[] = "%04X %02X%s%02X\n";

const char* DumpSID::getCredits()
{
    return ""
      "DumpSID 0.5\n"
      "(C) 2022 Benjamin Gerard\n";
}

DumpSID::DumpSID (sidbuilder *builder, int num, int fd, const char * fn) :
    sidemu(builder),
    m_num(num), m_fd(fd), m_fn(fn),
    m_model(0), m_sidfrq(0),
    m_boost(false)
{
    sidemu::reset();
}

DumpSID::~DumpSID()
{
}

void DumpSID::reset(uint8_t vol_and_filter)
{
#if defined(DEBUG) && DEBUG >= 10
    ::printf("DumpSID<%u>::reset(volume:%u)\n",
             unsigned(m_num), unsigned(vol_and_filter));
#endif
    m_accessClk = m_deltaClk = 0;

    if (eventScheduler) {
        dumpIni(0, byteAddr(0), vol_and_filter, m_model, m_sidfrq);
    }

        // GB: CAN NOT call clock() at this point
        // m_regs[0x18] = vol_and_filter;
        // dumpRel(0, byteAddr(0x18), " ", vol_and_filter);
}

void DumpSID::clock()
{
#if 0 && defined(DEBUG) && DEBUG >= 10
    ::printf("DumpSID<%u>::clock()\n", m_num);
#endif
    event_clock_t currentClk = eventScheduler->getTime(EVENT_CLOCK_PHI1);
    assert( currentClk >= m_accessClk );
    m_deltaClk  = currentClk - m_accessClk;
    m_accessClk = currentClk;
}

void DumpSID::dumpStr(const char * str, const int n)
{
    DumpSIDBuilder * const b = static_cast<DumpSIDBuilder*>(builder());
    if (b->getStatus())
      b->dumpStr(str, n);
}

void DumpSID::dumpFmt(const char * fmt, ...)
{
    char str[128];
    va_list vl;

    va_start(vl,fmt);
    int len = ::vsnprintf(str, sizeof(str), fmt, vl);
    va_end(vl);

    dumpStr(str, len);
}


void DumpSID::dumpRel(const unsigned clk, const int adr,
                      const char *dir, const int val)
{
    assert( (clk & 0xffff) == clk );
    assert( (adr & 0xFF) == adr );
    assert( (val & 0xFF) == val );
    dumpFmt(relfmt, clk, adr, dir, val);
}

void DumpSID::dumpReg(const uint8_t addr,
                      const char *dir, const uint8_t data)
{
    clock();
    const int adr( byteAddr(addr) ); // build byte address
    const unsigned relClk( m_deltaClk & 0xFFFF );
    const unsigned jmpClk( m_deltaClk >> 16 );
    if ( jmpClk ) {
        dumpIni(jmpClk, 0, 0, 0, 0);
    }
    dumpRel(relClk, adr, dir, data);
}

uint8_t DumpSID::read(uint_least8_t addr)
{
    addr &= 31;
    const uint8_t data = m_regs[addr & 31];

    switch (addr) {
        case 0x19: case 0x1A:
            // Paddle X and Y
            break;
        case 0x1B: case 0x1C:
            // GB: Voice# #3 Waveform and ADSR output.
            //
            // For now we can not dump the actual value. It would
            //     require at least a simple simulation. IDK if any
            //     player uses this value as input for other SID
            //     parameter (eg. filter cut-off).
            dumpReg(addr, ">", data);
            break;
    }
    return data;
}

void DumpSID::write(uint_least8_t addr, uint8_t data)
{
#if defined(DEBUG) && DEBUG >= 10
    ::printf("DumpSID<%u>::write($%04X,$%02X)\n", m_num, unsigned(addr), unsigned(data));
#endif
    addr &= 31;
    m_regs[addr] = data;
    if ( addr < 0x07 ) {
        if (m_muted & 1) return;        // Voice #1 muted
    }
    else if ( addr < 0x0E ) {
        if (m_muted & 2) return;        // Voice #2 muted
    }
    else if ( addr < 0x15 ) {
        if (m_muted & 4) return;        // voice #3 muted
    }
    else if ( addr < 0x18 ) {
        if (!m_filter) return;          // filter disabled
    }
    else if (addr == 0x18) {
        if (!m_filter) data &= 0x8F;    // filter disabled H/B?L bits
    }
    else return;                        // Ignore RO regsters
    dumpReg(addr, " ", data);
}

void DumpSID::voice(unsigned int num, bool mute)
{
    if (mute)
      m_muted |= 1u << num;
    else
      m_muted &= ~(1u << num);
#if defined(DEBUG) && DEBUG >= 10
    ::printf("DumpSID<%u>::voice(%u,%d) -> %u\n", m_num, num, int(mute), unsigned(m_muted));
#endif
}

void DumpSID::filter(bool enable)
{
#if defined(DEBUG) && DEBUG >= 10
  ::printf("DumpSID<%u>::filter(%d)\n", unsigned(m_num), int(enable));
#endif
    m_filter = enable;
}

// Set the emulated SID model
void DumpSID::model(SidConfig::sid_model_t model, bool digiboost)
{
    m_boost = digiboost;
    m_muted = 0;
    switch (model)
    {
        case SidConfig::MOS6581:
            m_model = 0x6581;
            break;
        case SidConfig::MOS8580:
            m_model = 0x8580;
            break;
        default:
            m_error.assign("invalid SID model");
            m_status = false;
            return;
    }
    m_status = true;
#if defined(DEBUG) && DEBUG >= 10
    ::printf("DumpSID<%u>::model(%i, %i)\n", m_num, m_model, int(m_boost));
#endif
}

void DumpSID::sampling(float sidfrq, float spr,
                       SidConfig::sampling_method_t m, bool fast)
{
    m_sidfrq = sidfrq;
#if defined(DEBUG) && DEBUG >= 10
    ::printf("DumpSID<%u>::sampling(sid<%.2f>)\n",
             m_num, sidfrq, spr);
#endif
    sidemu::sampling(sidfrq, spr, m, fast);
}

} // namespace libsidplayfp
