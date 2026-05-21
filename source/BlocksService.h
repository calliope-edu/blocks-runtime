#include "PxtShim.h"

#if MICROBIT_CODAL

#ifndef BLOCKS_SERVICE_H
#define BLOCKS_SERVICE_H

#include "MicroBitConfig.h"

#if CONFIG_ENABLED(DEVICE_BLE)

#include "MicroBit.h"
#include "MicroBitBLEManager.h"
#include "MicroBitBLEService.h"

#include "BlocksCommon.h"
#include "BlocksDevice.h"

// // Forward declaration
class BlocksDevice;

/**
 * Class definition for the the blocks editor basic Service.
 * Provides a BLE service for default extension of micro:bit in the blocks editor.
 */
class BlocksService : public MicroBitBLEService, MicroBitComponent {
public:
  // Buffer of characteristic for receiving commands.
  uint8_t commandChBuffer[BLOCKS_CH_BUFFER_SIZE_COMMAND] = {0};

  // Buffer of characteristic for sending data of GPIO and sensors state.
  uint8_t stateChBuffer[BLOCKS_CH_BUFFER_SIZE_STATE] = {0};

  // Buffer of characteristic for sending data about motion.
  uint8_t motionChBuffer[BLOCKS_CH_BUFFER_SIZE_MOTION] = {0};

  // Buffer of characteristic for sending pin events.
  uint8_t pinEventChBuffer[BLOCKS_CH_BUFFER_SIZE_NOTIFY] = {0};

  // Buffer of characteristic for sending action events.
  uint8_t actionEventChBuffer[BLOCKS_CH_BUFFER_SIZE_NOTIFY] = {0};

  // Buffer of characteristic for sending analog input values of P0.
  uint8_t analogInP0ChBuffer[BLOCKS_CH_BUFFER_SIZE_ANALOG_IN] = {0};

  // Buffer of characteristic for sending analog input values of P1.
  uint8_t analogInP1ChBuffer[BLOCKS_CH_BUFFER_SIZE_ANALOG_IN] = {0};

  // Buffer of characteristic for sending analog input values of P2.
  uint8_t analogInP2ChBuffer[BLOCKS_CH_BUFFER_SIZE_ANALOG_IN] = {0};

   // Buffer of characteristic for sending analog input values of P3.
  uint8_t analogInP3ChBuffer[BLOCKS_CH_BUFFER_SIZE_ANALOG_IN] = {0};

  // Buffer of characteristic for sending data.
  uint8_t dataChBuffer[BLOCKS_CH_BUFFER_SIZE_NOTIFY] = {0};

  /**
   * Constructor.
   * Create a representation of default extension for the blocks editor.
   */
  BlocksService();

  /**
   * Invoked when BLE connects.
   */
  void onConnect(const microbit_ble_evt_t *p_ble_evt);

  /**
   * Invoked when BLE disconnects.
   */
  void onDisconnect(const microbit_ble_evt_t *p_ble_evt);

  /**
   * Callback. Invoked when any of our attributes are written via BLE.
   */
  void onDataWritten(const microbit_ble_evt_write_t *params);

  /**
   * Callback. Invoked when any of our attributes are read via BLE.
   * Set  params->data and params->length to update the value
   */
  void onDataRead(microbit_onDataRead_t *params);

  /**
   * Periodic callback from MicroBit idle thread.
   */
  virtual void idleCallback();

  /**
   * @brief Notify action event.
   */
  void notifyActionEvent();

  /**
   * @brief Notify action event.
   */
  void notifyPinEvent();

  /**
   * @brief Notify sending data to the blocks editor
   * 
   */
  void notifyData();

  void notify();

  void update();

  /**
   * @brief Register data label and retrun ID for the label.
   *
   * @param dataLabel label to register
   * @param dataType type of the data to be received
   * @return int ID for the label
   */
  int registerWaitingDataLabel(ManagedString dataLabel, BlocksDataContentType dataType);

  /**
   * @brief Get type of content for the label
   *
   * @param labelID ID for the label
   * @return type of content [number | string]
   */
  BlocksDataContentType dataType(int labelID);

  /**
   * @brief Return content of the data as number
   *
   * @param labelID ID for the label
   * @return content of the data
   */
  float dataContentAsNumber(int labelID);

  /**
   * @brief Return content of the data as string
   *
   * @param labelID ID for the label
   * @return content of the data
   */
  ManagedString dataContentAsText(int labelID);

  /**
   * @brief Send a float with labele to the blocks editor.
   *  
   * @param dataLabel label of the data
   * @param dataContent content of the data
   */
  void sendNumberWithLabel(ManagedString dataLabel, float dataContent);

  /**
   * @brief Send a string with labele to the blocks editor.
   * 
   * @param dataLabel label of the data
   * @param dataContent content of the data
   */
  void sendTextWithLabel(ManagedString dataLabel, ManagedString dataContent);

private:
  /**
   * @brief micro:bit runtime object.
   *
   */
  MicroBit &uBit;

  /**
   * @brief Microbit More object.
   *
   */
  BlocksDevice *blocks;

  // Index for each charactersitic in arrays of handles and UUIDs
  typedef enum blocks_cIdx
  {
    blocks_cIdx_COMMAND,
    blocks_cIdx_STATE,
    blocks_cIdx_MOTION,
    blocks_cIdx_PIN_EVENT,
    blocks_cIdx_ACTION_EVENT,
    blocks_cIdx_ANALOG_IN_P0,
    blocks_cIdx_ANALOG_IN_P1,
    blocks_cIdx_ANALOG_IN_P2,
    blocks_cIdx_ANALOG_IN_P3,
    blocks_cIdx_DATA,
    blocks_cIdx_COUNT
  } blocks_cIdx;

  // UUIDs for our service and characteristics
  static const uint8_t baseUUID[16];
  static const uint16_t serviceUUID;
  static const uint16_t charUUID[blocks_cIdx_COUNT];

  // Data for each characteristic when they are held by Soft Device.
  MicroBitBLEChar chars[blocks_cIdx_COUNT];

  /**
   * Write IO characteristics.
   */
  void writeDigitalIn();

public:
  int characteristicCount() { return blocks_cIdx_COUNT; };
  MicroBitBLEChar *characteristicPtr(int idx) { return &chars[idx]; };
};

#endif // CONFIG_ENABLED(DEVICE_BLE)
#endif // BLOCKS_SERVICE_H
#endif // MICROBIT_CODAL
