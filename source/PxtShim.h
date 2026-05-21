/**
 * Minimal pxt.h shim — replaces the MakeCode-runtime pxt header that
 * pxt-blocks's BlocksService/BlocksDevice originally used.
 *
 * The full pxt.h pulls in MakeCode's TypeScript value box / GC / shim
 * machinery, none of which is reachable from a Blocks editor that drives
 * the device over MbitMore BLE + framed USB serial. So we only re-expose
 * the small pxt-namespace surface that BlocksService.cpp + BlocksDevice.cpp
 * actually touch:
 *
 *   - pxt::uBit       — the global MicroBit instance (defined in main.cpp)
 *   - MICROBIT_CODAL  — codal vs DAL build flag (forced to 1 here; this
 *                       runtime is CODAL-only)
 *   - pxt::getMicrophoneLevel() — codal LevelDetectorSPL pointer for the
 *                       microphone-level path (one call in BlocksDevice).
 */

#ifndef BLOCKS_RUNTIME_PXT_SHIM_H
#define BLOCKS_RUNTIME_PXT_SHIM_H

// Hard-code: this runtime ships CODAL-only (V3 / Calliope mini 3).
// pxt-blocks's BlocksServiceDAL.h is intentionally not built here; the
// `#if MICROBIT_CODAL` guards inside the Blocks sources pick the CODAL
// path everywhere.
#ifndef MICROBIT_CODAL
#define MICROBIT_CODAL 1
#endif

#include "MicroBit.h"
#include "LevelDetectorSPL.h"

namespace pxt {
    /** Defined in source/main.cpp. */
    extern MicroBit uBit;

    /**
     * Lazily-initialised microphone level detector, attached to the
     * MicroBit's audio pipeline. Returns nullptr until the first call has
     * had a chance to wire up the SPL detector.
     */
    codal::LevelDetectorSPL* getMicrophoneLevel();
}

// pxt-blocks's original C++ uses unqualified `uBit` (the full pxt.h does
// `using namespace pxt;` at translation-unit level). Mirror that so the
// lifted Blocks*.cpp files don't need to change.
using pxt::uBit;

#endif
