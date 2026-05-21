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

// Period for the sensor-broadcaster fiber that fills the MbitMore STATE
// characteristic. Matches pxt-blocks's original UPDATE_PERIOD in
// Blocks.cpp — chosen short enough that scratch-vm sees fresh sensor
// data within one VM tick (~30 ms), long enough not to monopolise the
// codal scheduler.
static const int UPDATE_PERIOD_MS = 19;

// Sensor-broadcaster fiber. The pxt-blocks original spawned this from
// `Blocks::startBlocksService()`; we instead spawn it directly from
// main() because we don't have the pxt-Blocks-namespace shim layer.
//
// Without this fiber, BlocksService::update() never runs → the STATE
// characteristic stays all-zero → the campus widget's `probeBle()` in
// program-type.ts can't distinguish a real blocks runtime from the
// "CODAL stub" case it explicitly guards against, and the device
// looks like an unknown program even though MbitMore is advertised.
static BlocksService *s_blocks_service = nullptr;

static void blocksUpdateFiber() {
    while (s_blocks_service != nullptr) {
        s_blocks_service->update();
        fiber_sleep(UPDATE_PERIOD_MS);
    }
}

int main() {
    pxt::uBit.init();

    // BlocksService constructor registers the MbitMore GATT service +
    // characteristics; BlocksDevice singleton wires up the per-channel
    // sensor/actuator handlers.
    static BlocksService svc;
    s_blocks_service = &svc;
    BlocksDevice::getInstance();

    // Spawn the broadcaster fiber that calls svc.update() every
    // UPDATE_PERIOD_MS. Without this STATE never gets filled and the
    // widget can't detect the runtime over BLE (see comment above).
    create_fiber(blocksUpdateFiber);

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
