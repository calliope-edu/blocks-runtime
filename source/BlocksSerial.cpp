#include "BlocksCommon.h"
#if BLOCKS_USE_SERIAL

#include "BlocksSerial.h"

static BlocksSerial *serial; // Hold it as a static pointer to be called by create_fiber().

/**
 * @brief Start a process to receive data.
 * 
 */
void startBlocksSerialReceiving() {
  serial->startSerialReceiving();
}

/**
 * @brief Start a process to update sensor data.
 * 
 */
void startBlocksSerialUpdating() {
  serial->startSerialUpdating();
}

/**
 * @brief Read one byte from RX. Current fiber sleeps until when received a data.
 * 
 * @return uint8_t Data read from RX
 */
uint8_t readSync() {
  fiber_sleep(1); // Need to prevent from freezing
  return uBit.serial.read(SYNC_SLEEP);
}

/**
 * @brief Calculate checksum of the data. Sum of the buffer and return the remainder which deviced by 0xFF. 
 * 
 * @param buff Buffer to be calculate
 * @param len Length of buffer
 * @return uint8_t Number of checksum
 */
uint8_t chksum8(const uint8_t *buff, size_t len) {
  unsigned int sum;
  for (sum = 0; len != 0; len--) {
    sum += *(buff++);
  }
  return (uint8_t)(sum % 0xFF);
}

BlocksSerial::BlocksSerial(BlocksDevice &_blocks) : blocks(_blocks) {
  uBit.log.setSerialMirroring(false); // stop log using serial
  serial = this;
  // Baud rate
  // int rate = 57600;
  int rate = 115200; // Default for micro:bit
#if MICROBIT_CODAL
  uBit.serial.setBaud(rate);
#else
  uBit.serial.baud((int)rate);
#endif
  create_fiber(startBlocksSerialReceiving);
}

void BlocksSerial::readResponseOnSerial(uint16_t ch, uint8_t *dataBuffer, size_t len) {
  size_t frameSize = 6 + len;
  uint8_t frame[frameSize] = {0};
  frame[0] = BLOCKS_SFD;
  frame[1] = ChResponse::RES_READ;
  frame[2] = ch >> 8;
  frame[3] = ch & 0x00FF;
  frame[4] = len;
  memcpy(&frame[5], dataBuffer, len);
  frame[frameSize - 1] = chksum8(frame, frameSize - 1);
  while ((BLOCKS_TX_BUFFER_SIZE - uBit.serial.txBufferedSize()) < (int)frameSize) {
    fiber_sleep(1);
  }
  uBit.serial.send(frame, frameSize, ASYNC);
}

void BlocksSerial::writeResponseOnSerial(uint16_t ch, bool response) {
  uint8_t frame[7] = {0};
  frame[0] = BLOCKS_SFD;
  frame[1] = ChResponse::RES_WRITE;
  frame[2] = ch >> 8;
  frame[3] = ch & 0x00FF;
  frame[4] = 1;
  frame[5] = 1;
  frame[6] = chksum8(frame, 6);
  while ((BLOCKS_TX_BUFFER_SIZE - uBit.serial.txBufferedSize()) < 7) {
    fiber_sleep(1);
  }
  uBit.serial.send(frame, 7, SYNC_SLEEP);
}

void BlocksSerial::notifyOnSerial(uint16_t ch, uint8_t *dataBuffer, size_t len) {
  size_t frameSize = 6 + len;
  uint8_t frame[frameSize] = {0};
  frame[0] = BLOCKS_SFD;
  frame[1] = ChResponse::RES_NOTIFY;
  frame[2] = ch >> 8;
  frame[3] = ch & 0x00FF;
  frame[4] = len;
  memcpy(&frame[5], dataBuffer, len);
  frame[frameSize - 1] = chksum8(frame, frameSize - 1);
  while ((BLOCKS_TX_BUFFER_SIZE - uBit.serial.txBufferedSize()) < (int)frameSize) {
    fiber_sleep(1);
  }
  uBit.serial.send(frame, frameSize, ASYNC);
}

void BlocksSerial::startSerialUpdating() {
  BlocksService *moreService = blocks.moreService;
  uint16_t stateCh = 0x0101;
  uint16_t motionCh = 0x0102;
  while (true) {
    if (uBit.serial.txBufferedSize() < 100) {
      blocks.updateState(moreService->stateChBuffer);
      readResponseOnSerial(stateCh, moreService->stateChBuffer, BLOCKS_CH_BUFFER_SIZE_STATE);
      fiber_sleep(20);
      blocks.updateMotion(moreService->motionChBuffer);
      readResponseOnSerial(motionCh, moreService->motionChBuffer, BLOCKS_CH_BUFFER_SIZE_MOTION);
      fiber_sleep(20);
    }
  }
}

void BlocksSerial::startSerialReceiving() {
  BlocksService *moreService = blocks.moreService;
  int requestType;
  uint16_t ch;
  uint8_t *responseBuffer;

  uBit.serial.setTxBufferSize(BLOCKS_TX_BUFFER_SIZE);
  uBit.serial.clearTxBuffer();
  uBit.serial.setRxBufferSize(BLOCKS_RX_BUFFER_SIZE);
  uBit.serial.clearRxBuffer();

  uint8_t frame[26] = {0};
  size_t frameReceived = 0;

  while (true) {
    while ((frameReceived > 0) && (BLOCKS_SFD != frame[0])) {
      frameReceived--;
      memmove(frame, frame + 1, frameReceived);
    }
    if (frameReceived == 0) {
      frame[0] = readSync();
      if (BLOCKS_SFD != frame[0]) {
        continue;
      }
      frameReceived = 1;
    }
    if (frameReceived == 1) {
      frame[1] = readSync();
      frameReceived = 2;
    }
    requestType = frame[1];
    if (requestType < 0 || requestType > ChRequest::REQ_NOTIFY_START) {
      frameReceived--;
      memmove(frame, frame + 1, frameReceived);
      continue; // reset frame reading
    }
    if (frameReceived == 2) {
      frame[2] = readSync();
      frameReceived = 3;
    }
    if (frameReceived == 3) {
      frame[3] = readSync();
      frameReceived = 4;
    }
    ch = frame[2] << 8;
    ch |= frame[3];
    // COMMAND
    if (0x0100 == ch) {
      if (ChRequest::REQ_READ == requestType) {
        // Start connection
        blocks.updateVersionData();
        responseBuffer = moreService->commandChBuffer;
        responseBuffer[2] = BlocksCommunicationRoute::SERIAL;
        readResponseOnSerial(ch, responseBuffer, BLOCKS_CH_BUFFER_SIZE_COMMAND);
        if (!blocks.serialConnected) {
          blocks.onSerialConnected();
          create_fiber(startBlocksSerialUpdating);
        }
        frameReceived = 0; // reset frame reading
        continue;
      }
      if (ChRequest::REQ_WRITE == requestType || ChRequest::REQ_WRITE_RESPONSE == requestType) {
        if (frameReceived == 4) {
          frame[4] = readSync();
          frameReceived = 5;
        }
        uint8_t commandLength = frame[4];
        if (commandLength > 20) {
          frameReceived--;
          memmove(frame, frame + 1, frameReceived);
          continue;
        }
        size_t frameSize = 5 + commandLength + 1;
        for (size_t i = frameReceived; i < frameSize; i++) {
          frame[i] = readSync();
          frameReceived = i + 1;
        }
        if (chksum8(frame, 5 + commandLength) != frame[frameSize - 1]) {
          frameReceived--;
          memmove(frame, frame + 1, frameReceived);
          continue;
        }
        memcpy(moreService->commandChBuffer, &frame[5], commandLength);
        blocks.onCommandReceived(moreService->commandChBuffer, commandLength);
        if (ChRequest::REQ_WRITE_RESPONSE == requestType) {
          writeResponseOnSerial(ch, true);
        }
        frameReceived = 0; // reset frame reading
        continue;
      }
    }

    // State
    if (0x0101 == ch) {
      if (ChRequest::REQ_READ == requestType) {
        blocks.updateState(moreService->stateChBuffer);
        readResponseOnSerial(0x0101, moreService->stateChBuffer, BLOCKS_CH_BUFFER_SIZE_STATE);
        frameReceived = 0; // reset frame reading
        continue;
      }
    }

    // Motion
    if (0x0102 == ch) {
      if (ChRequest::REQ_READ == requestType) {
        blocks.updateMotion(moreService->motionChBuffer);
        readResponseOnSerial(0x0102, moreService->motionChBuffer, BLOCKS_CH_BUFFER_SIZE_MOTION);
        frameReceived = 0; // reset frame reading
        continue;
      }
    }

    // ANALOG_IN_P0
    if (0x0120 == ch) {
      if (ChRequest::REQ_READ == requestType) {
        blocks.updateAnalogIn(moreService->analogInP0ChBuffer, 0);
        readResponseOnSerial(ch, moreService->analogInP0ChBuffer, BLOCKS_CH_BUFFER_SIZE_ANALOG_IN);
        frameReceived = 0; // reset frame reading
        continue;
      }
    }

    // ANALOG_IN_P1
    if (0x0121 == ch) {
      if (ChRequest::REQ_READ == requestType) {
        blocks.updateAnalogIn(moreService->analogInP1ChBuffer, 1);
        readResponseOnSerial(ch, moreService->analogInP1ChBuffer, BLOCKS_CH_BUFFER_SIZE_ANALOG_IN);
        frameReceived = 0; // reset frame reading
        continue;
      }
    }

    // ANALOG_IN_P2
    if (0x0122 == ch) {
      if (ChRequest::REQ_READ == requestType) {
        blocks.updateAnalogIn(moreService->analogInP2ChBuffer, 2);
        readResponseOnSerial(ch, moreService->analogInP2ChBuffer, BLOCKS_CH_BUFFER_SIZE_ANALOG_IN);
        frameReceived = 0; // reset frame reading
        continue;
      }
    }

    // ANALOG_IN_P3
    if (0x0123 == ch) {
      if (ChRequest::REQ_READ == requestType) {
        blocks.updateAnalogIn(moreService->analogInP3ChBuffer, 3);
        readResponseOnSerial(ch, moreService->analogInP3ChBuffer, BLOCKS_CH_BUFFER_SIZE_ANALOG_IN);
        frameReceived = 0; // reset frame reading
        continue;
      }
    }

    // Not matched
    frameReceived--;
    memmove(frame, frame + 1, frameReceived);
  }
}

#endif // BLOCKS_USE_SERIAL