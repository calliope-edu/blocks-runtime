/**
 * blocks-runtime — direct codal entry point for the Calliope Blocks editor.
 *
 * Replaces the pxt-microbit-V5 build pipeline that used to ship blocks.hex.
 * No MakeCode, no TypeScript runtime — just init codal, start the
 * BlocksService BLE + USB serial proxy, then enter the codal scheduler.
 *
 * The actual program logic lives in the browser (scratch-vm); this device
 * binary is a fixed sensor/actuator proxy that exposes the MbitMore
 * binary protocol (see BlocksService.cpp + BlocksSerial.cpp).
 */

#include "MicroBit.h"
#include "BlocksService.h"
#include "BlocksDevice.h"
#include "BlocksSerial.h"
#include "LevelDetectorSPL.h"

namespace pxt {
    /** The global MicroBit instance. BlocksService grabs this via the
     *  PxtShim header so the original pxt-blocks .cpp keeps compiling. */
    MicroBit uBit;

    /** Cache for the microphone-level detector, attached lazily on first
     *  call from BlocksDevice's microphone path. */
    static codal::LevelDetectorSPL* s_level = nullptr;

    codal::LevelDetectorSPL* getMicrophoneLevel() {
        if (s_level == nullptr) {
            // Mic + level detector aren't started by default to save
            // power. Initialise on first request and keep the pointer
            // for subsequent calls. `audio.levelSPL` is already a
            // LevelDetectorSPL* on codal v0.3.5, so no address-of.
            s_level = uBit.audio.levelSPL;
        }
        return s_level;
    }
}

// Unqualified `uBit` resolves via the `using pxt::uBit;` declaration at
// the bottom of PxtShim.h — pxt-blocks's lifted .cpp files rely on that.

int main() {
    pxt::uBit.init();

    // The Blocks scratch runtime spends almost all its life in the codal
    // fiber scheduler waiting for BLE writes or USB serial frames. The
    // BlocksService constructor registers GATT characteristics and the
    // BlocksDevice singleton wires up the per-channel handlers. Both
    // happen lazily on first reference, but instantiating them here makes
    // the boot order explicit + lets us hold a reference if we ever need
    // to introspect state from the watchdog.
    static BlocksService svc;
    BlocksDevice::getInstance();

    // Boot animation — a heart, matching microbit-v2-samples convention.
    // Visible confirmation that codal init succeeded; scratch-vm will
    // overwrite the matrix once it connects and sends its first
    // displayMatrix command.
    static const uint8_t heart[] = {
        0, 1, 0, 1, 0,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        0, 1, 1, 1, 0,
        0, 0, 1, 0, 0,
    };
    MicroBitImage img(5, 5, heart);
    pxt::uBit.display.print(img);

    // Hand control to the codal scheduler. We never return.
    while (true) {
        pxt::uBit.sleep(1000);
    }
}
