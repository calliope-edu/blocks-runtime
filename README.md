# blocks-runtime

Direct codal/CMake build of the Calliope Blocks editor's on-device runtime.
Replaces the `pxt-microbit-V5` → `pxt-blocks` build pipeline that used to
ship `mini-connection-widget/src/assets/blocks.hex`.

## Why this exists

The Blocks editor's on-device behaviour is a fixed C++ binary:

- exposes the **MbitMore** BLE service (UUID base `0b50f3e4-...`) so
  scratch-vm in the browser can drive the device,
- exposes the same **MbitMore binary protocol** over USB CDC serial (115200,
  framed `[SFD][type][ch_hi][ch_lo][len][payload][chksum]`).

Scratch programs are interpreted **in the browser** by scratch-vm. The
device never runs MakeCode-style blocks, TypeScript, or any
per-project-compiled code. So all the pxt machinery (TS→blocks, extension
graph, project assembly, universal-hex packaging) was overhead — every
Blocks user gets the same `blocks.hex`.

This repo strips that pipeline down to **codal + cmake + a single static
binary**. Benefits:

- One codal version across MicroPython + MakeCode + Blocks
  (`v0.3.5-campus-open-1`) — same bootloader, same MTU 247 DFU, same
  BD_ADDR-keep-app, same open-mode BLE settings.
- Build time: ~3 min vs ~20+ min on pxt-microbit-V5.
- Single artifact, V3-only (no universal hex).
- Debuggable with arm-none-eabi-gdb against a single .elf + .map.

## Layout

```
blocks-runtime/
├── README.md            — this file
├── codal.json           — target = codal-microbit-v2 @ v0.3.5-campus-open-1, BLE config
├── CMakeLists.txt       — codal driver, copied from microbit-v2-samples
├── build.py             — convenience wrapper (`python build.py`)
├── source/
│   ├── main.cpp         — boots MicroBit, instantiates BlocksService, shows heart
│   ├── PxtShim.h        — minimal pxt.h replacement (just pxt::uBit + getMicrophoneLevel)
│   ├── BlocksCommon.h
│   ├── BlocksService.{cpp,h}   — MbitMore BLE service (lifted from pxt-blocks, includes patched to PxtShim)
│   ├── BlocksDevice.{cpp,h}    — I/O fanout, sensor handlers
│   └── BlocksSerial.{cpp,h}    — USB CDC framed transport
└── utils/               — codal cmake helpers, vendored from microbit-v2-samples

```

## Build

```bash
cd FIRMWARE/blocks-runtime
python build.py        # downloads codal-microbit-v2 into libraries/, builds MICROBIT.hex
```

On a clean tree the first build clones codal-microbit-v2 + its sublibraries
(~5 min). Incremental builds are ~10-30 s.

Output: `./MICROBIT.hex`. Drop that into
`mini-connection-widget/src/assets/blocks.hex` to ship.

### WSL note

If you're on Windows: build **in the WSL native filesystem**
(`~/blocks-runtime` or similar). Building from `/mnt/c/...` is 20× slower
because of the 9P bridge — see [`../micropython-calliope-mini-v3`'s
build notes for the same trap].

## Where it came from

The C++ sources in `source/Blocks*.{cpp,h}` are lifted from
[`pxt-blocks`](../../pxt-blocks/) (commit captured at the time of the
initial commit of this repo). They are CODAL-only — the
`BlocksServiceDAL.{cpp,h}` files from pxt-blocks are not carried over;
the `#if MICROBIT_CODAL` guards inside each file pick the CODAL path
unconditionally because `PxtShim.h` hardcodes `MICROBIT_CODAL = 1`.

`PxtShim.h` is the only file that's new vs pxt-blocks: it provides the
small `pxt::` namespace surface (`pxt::uBit`, `pxt::getMicrophoneLevel`,
`MICROBIT_CODAL`) that the Blocks sources expected from the full MakeCode
runtime header.

## Codal version

`v0.3.5-campus-open-1` is the calliope-edu fork tag that bundles:

- The `lib/bootloader.o` rebuild with **BD_ADDR keep-app** (Web Bluetooth
  can reconnect to the bootloader at the app's MAC) + **MTU 247**
  (244-byte DFU writes, ~4× DFU speedup).
- All campus-open BLE security work (open-mode no-bonding, the
  `MICROBIT_BLE_OPEN=1` + `MICROBIT_BLE_SECURITY_LEVEL` security_mode
  override fix from codal v0.3.5).
- 292 commits ahead of v0.3.5 mainline.

If you ever need to test against the upstream Lancaster build, point
`codal.json:5` at `lancaster-university/codal-microbit-v2` and pick a
v0.3.5 tag.

## Known caveats

1. **PxtShim.h's `getMicrophoneLevel()` returns `&uBit.audio.levelSPL`
   directly.** That's the codal-microbit-v2 v0.3.5 API path — confirm the
   audio pipeline is auto-started by `uBit.init()` (it is, when
   `DEVICE_BLE=1` and the audio dependencies link in). If the BlocksDevice
   microphone path returns zeros, audit `BlocksDevice.cpp:26` and verify
   the codal v0.3.5 audio API matches what the old v0.2.40-line code
   expected.
2. **No yotta config translation.** The pxt-microbit-V5 build wrote yotta
   config into `microbit-dal.bluetooth.*`. We're CODAL-only now, so those
   are out — `codal.json` directly sets the equivalent CODAL `config.*`
   defines.
3. **No multi-variant output.** This is a V3-only hex. If you ever need a
   V1/V2 (DAL) variant for older hardware, build it from the older
   pxt-blocks pipeline as a one-off — don't try to retrofit DAL into this
   repo.

## Status

**Scaffold only — not yet built or smoke-tested.** First end-to-end
verification step:

1. `python build.py` to clone codal-microbit-v2 + build MICROBIT.hex.
2. Flash to a Calliope mini 3 via USB; expect heart on the display + BLE
   advertising with MbitMore (UUID `0b50f3e4-...`) + open-mode no-bonding.
3. Open scratch-gui (Blocks editor), connect to the mini, run a simple
   "set pixel + display block" program. Validates the BLE/USB protocol
   layer.
