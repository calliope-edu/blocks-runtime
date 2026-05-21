#include "PxtShim.h"

#if MICROBIT_CODAL

#include "nrf_saadc.h"

/**
 * Class definition for the the blocks editor Blocks Service.
 * Provides a BLE service to the blocks editor3.
 */
#include "MicroBitConfig.h"

#if CONFIG_ENABLED(DEVICE_BLE)

// #include "ble_advdata.h"

#include "BlocksService.h"
#include "MicroBitButton.h"
#include "ble_advdata.h"

// service ID: 0b50f3e4-607f-4151-9091-7d008d6ffc5c
const uint8_t BlocksService::baseUUID[16] = {0x0b, 0x50, 0xf3, 0xe4, 0x60, 0x7f, 0x41, 0x51, 0x90, 0x91, 0x7d, 0x00, 0x8d, 0x6f, 0xfc, 0x5c};
const uint16_t BlocksService::serviceUUID = 0xf3e4;
const uint16_t BlocksService::charUUID[blocks_cIdx_COUNT] = {
    0x0100, // COMMAND
    0x0101, // STATE
    0x0102, // MOTION
    0x0110, // PIN_EVENT
    0x0111, // ACTION_EVENT
    0x0120, // ANALOG_IN_P0
    0x0121, // ANALOG_IN_P1
    0x0122, // ANALOG_IN_P2
    0x0123, // ANALOG_IN_P3
    0x0130  // MESSAGE
};


// SECURITY_MODE_ENCRYPTION_NO_MITM = 1
// SECURITY_MODE_ENCRYPTION_OPEN_LINK = 2
// SECURITY_MODE_ENCRYPTION_WITH_MITM = 3

/**
 * Constructor.
 * Create a representation of default extension for the blocks editor.
 */
BlocksService::BlocksService() : uBit(pxt::uBit) {
  blocks = &BlocksDevice::getInstance();
  blocks->moreService = this;

  // Create the service.
  bs_uuid_type = BLE_UUID_TYPE_UNKNOWN;
  RegisterBaseUUID(baseUUID);
  CreateService(serviceUUID);

  // Add each of our characteristics.
  CreateCharacteristic(
      blocks_cIdx_COMMAND,
      charUUID[blocks_cIdx_COMMAND],
      (uint8_t *)(commandChBuffer),
      BLOCKS_CH_BUFFER_SIZE_COMMAND,
      BLOCKS_CH_BUFFER_SIZE_COMMAND,
      microbit_propWRITE | microbit_propWRITE_WITHOUT | microbit_propREAD);
    

  CreateCharacteristic(
      blocks_cIdx_STATE,
      charUUID[blocks_cIdx_STATE],
      (uint8_t *)(stateChBuffer),
      BLOCKS_CH_BUFFER_SIZE_STATE,
      BLOCKS_CH_BUFFER_SIZE_STATE,
      microbit_propREAD);

  CreateCharacteristic(
      blocks_cIdx_MOTION,
      charUUID[blocks_cIdx_MOTION],
      (uint8_t *)(motionChBuffer),
      BLOCKS_CH_BUFFER_SIZE_MOTION,
      BLOCKS_CH_BUFFER_SIZE_MOTION,
      microbit_propREAD);

  CreateCharacteristic(
      blocks_cIdx_PIN_EVENT,
      charUUID[blocks_cIdx_PIN_EVENT],
      (uint8_t *)(pinEventChBuffer),
      BLOCKS_CH_BUFFER_SIZE_NOTIFY,
      BLOCKS_CH_BUFFER_SIZE_NOTIFY,
      microbit_propREAD | microbit_propNOTIFY);

  CreateCharacteristic(
      blocks_cIdx_ACTION_EVENT,
      charUUID[blocks_cIdx_ACTION_EVENT],
      (uint8_t *)(actionEventChBuffer),
      BLOCKS_CH_BUFFER_SIZE_NOTIFY,
      BLOCKS_CH_BUFFER_SIZE_NOTIFY,
      microbit_propREAD | microbit_propNOTIFY);

  CreateCharacteristic(
      blocks_cIdx_ANALOG_IN_P0,
      charUUID[blocks_cIdx_ANALOG_IN_P0],
      (uint8_t *)(analogInP0ChBuffer),
      BLOCKS_CH_BUFFER_SIZE_ANALOG_IN,
      BLOCKS_CH_BUFFER_SIZE_ANALOG_IN,
      microbit_propREAD | microbit_propREADAUTH);

  CreateCharacteristic(
      blocks_cIdx_ANALOG_IN_P1,
      charUUID[blocks_cIdx_ANALOG_IN_P1],
      (uint8_t *)(analogInP1ChBuffer),
      BLOCKS_CH_BUFFER_SIZE_ANALOG_IN,
      BLOCKS_CH_BUFFER_SIZE_ANALOG_IN,
      microbit_propREAD | microbit_propREADAUTH);

  CreateCharacteristic(
      blocks_cIdx_ANALOG_IN_P2,
      charUUID[blocks_cIdx_ANALOG_IN_P2],
      (uint8_t *)(analogInP2ChBuffer),
      BLOCKS_CH_BUFFER_SIZE_ANALOG_IN,
      BLOCKS_CH_BUFFER_SIZE_ANALOG_IN,
      microbit_propREAD | microbit_propREADAUTH);

  CreateCharacteristic(
      blocks_cIdx_ANALOG_IN_P3,
      charUUID[blocks_cIdx_ANALOG_IN_P3],
      (uint8_t *)(analogInP3ChBuffer),
      BLOCKS_CH_BUFFER_SIZE_ANALOG_IN,
      BLOCKS_CH_BUFFER_SIZE_ANALOG_IN,
      microbit_propREAD | microbit_propREADAUTH);

  CreateCharacteristic(
      blocks_cIdx_DATA,
      charUUID[blocks_cIdx_DATA],
      (uint8_t *)(dataChBuffer),
      BLOCKS_CH_BUFFER_SIZE_NOTIFY,
      BLOCKS_CH_BUFFER_SIZE_NOTIFY,
      microbit_propREAD | microbit_propNOTIFY);

  // Stop advertising.
  // uBit.ble->stopAdvertising();

  // Configure advertising.
  // ble_uuid_t adv_uuids[] = {{serviceUUID, BLE_UUID_TYPE_BLE}};
  // ble_advdata_t advdata;
  // memset(&advdata, 0, sizeof(advdata));
  // advdata.name_type = BLE_ADVDATA_FULL_NAME;
  // advdata.include_appearance = true;
  // advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
  // advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
  // advdata.uuids_complete.p_uuids = adv_uuids;
  // bool connectable = true;
  // bool discoverable = true;
  // bool whitelist = false;
  // uBit.ble->configureAdvertising(connectable, discoverable, whitelist,
  //                                MICROBIT_BLE_ADVERTISING_INTERVAL,
  //                                MICROBIT_BLE_ADVERTISING_TIMEOUT, &advdata);

  // Configure advertising.
  // bool connectable = true;
  // bool discoverable = true;
  // bool whitelist = false;
  // uint16_t interval_ms = MICROBIT_BLE_ADVERTISING_INTERVAL;
  // int timeout_seconds = MICROBIT_BLE_ADVERTISING_TIMEOUT;

  // ble_advdata_t advdata;
  // memset(&advdata, 0, sizeof(advdata));
  // advdata.name_type = BLE_ADVDATA_FULL_NAME;
  // advdata.include_appearance = true;
  // advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

  // ble_uuid_t adv_uuids[] = {{serviceUUID, BLE_UUID_TYPE_BLE}};
  // advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
  // advdata.uuids_complete.p_uuids = adv_uuids;

  // uBit.ble->configureAdvertising(connectable, discoverable, whitelist, interval_ms, timeout_seconds, &advdata);
  // uBit.ble->configureAdvertising(connectable, discoverable, whitelist, interval_ms, timeout_seconds);

  // Start advertising.
  // uBit.ble->advertise();

  fiber_add_idle_component(this);
}

/**
 * Invoked when BLE connects.
 */
void BlocksService::onConnect(const microbit_ble_evt_t *p_ble_evt) {
  blocks->updateVersionData();
}

/**
 * Invoked when BLE disconnects.
 */
void BlocksService::onDisconnect(const microbit_ble_evt_t *p_ble_evt) {
  // uBit.ble->advertise();
}

/**
 * Callback. Invoked when any of our attributes are written via BLE.
 */
void BlocksService::onDataWritten(const microbit_ble_evt_write_t *params) {
  if (params->handle == valueHandle(blocks_cIdx_COMMAND) && params->len > 0) {
    blocks->onCommandReceived((uint8_t *)params->data, params->len);
  }
}

/**
 * Callback. Invoked when any of our attributes are read via BLE.
 * Set  params->data and params->length to update the value
 */
void BlocksService::onDataRead(microbit_onDataRead_t *params) {
  if (params->handle == valueHandle(blocks_cIdx_ANALOG_IN_P0)) {
    blocks->updateAnalogIn(analogInP0ChBuffer, 0);
    params->data = analogInP0ChBuffer;
    params->length = 2;
  } else if (params->handle == valueHandle(blocks_cIdx_ANALOG_IN_P1)) {
    blocks->updateAnalogIn(analogInP1ChBuffer, 1);
    params->data = analogInP1ChBuffer;
    params->length = 2;
  } else if (params->handle == valueHandle(blocks_cIdx_ANALOG_IN_P2)) {
    blocks->updateAnalogIn(analogInP2ChBuffer, 2);
    params->data = analogInP2ChBuffer;
    params->length = 2;
  } else if (params->handle == valueHandle(blocks_cIdx_ANALOG_IN_P3)) {
    blocks->updateAnalogIn(analogInP3ChBuffer, 3);
    params->data = analogInP3ChBuffer;
    params->length = 2;
  }
}

/**
 * Periodic callback from MicroBit idle thread.
 */
void BlocksService::idleCallback() {
  // No-op: do not commandeer the display while disconnected.
}

/**
 * @brief Notify action event.
 */
void BlocksService::notifyActionEvent() {
  if (!getConnected())
    return;
  notifyChrValue(blocks_cIdx_ACTION_EVENT, actionEventChBuffer,
                 BLOCKS_CH_BUFFER_SIZE_NOTIFY);
}

/**
 * @brief Notify pin event.
 */
void BlocksService::notifyPinEvent() {
  if (!getConnected())
    return;
  notifyChrValue(blocks_cIdx_PIN_EVENT, pinEventChBuffer,
                 BLOCKS_CH_BUFFER_SIZE_NOTIFY);
}

/**
 * Notify message to the blocks editor3
 */
void BlocksService::notifyData() {
  if (!getConnected())
    return;
  notifyChrValue(blocks_cIdx_DATA, dataChBuffer, BLOCKS_CH_BUFFER_SIZE_NOTIFY);
}

/**
 * Notify data to the blocks editor3
 */
void BlocksService::notify() {}

/**
 * Update all sensors.
 */
void BlocksService::update() {
  if (getConnected()) {
    blocks->updateState(stateChBuffer);
    blocks->updateMotion(motionChBuffer);
  }
}

/**
 * @brief Register data label and retrun ID for the label.
 *
 * @param dataLabel label to register
 * @param dataType type of the data to be received
 * @return int ID for the label
 */
int BlocksService::registerWaitingDataLabel(ManagedString dataLabel, BlocksDataContentType dataType) {
  return blocks->registerWaitingDataLabel(dataLabel, dataType);
}

/**
 * @brief Get type of content for the label
 *
 * @param labelID ID for the label
 * @return type of content [number | string]
 */
BlocksDataContentType BlocksService::dataType(int labelID) {
  return blocks->dataType(labelID);
}

/**
 * @brief Return content of the data as number
 *
 * @param labelID ID for the label
 * @return content of the data
 */
float BlocksService::dataContentAsNumber(int labelID) {
  return blocks->dataContentAsNumber(labelID);
}

/**
 * @brief Return content of the data as string
 *
 * @param labelID ID for the label
 * @return content of the data
 */
ManagedString BlocksService::dataContentAsText(int labelID) {
  return blocks->dataContentAsText(labelID);
}

/**
 * @brief Send a float with label to the blocks editor.
 *  
 * @param dataLabel label of the data
 * @param dataContent content of the data
 */
void BlocksService::sendNumberWithLabel(ManagedString dataLabel, float dataContent) {
  blocks->sendNumberWithLabel(dataLabel, dataContent);
}

/**
 * @brief Send a string with label to the blocks editor.
 * 
 * @param dataLabel label of the data
 * @param dataContent content of the data
 */
void BlocksService::sendTextWithLabel(ManagedString dataLabel, ManagedString dataContent) {
  blocks->sendTextWithLabel(dataLabel, dataContent);
}

#endif // CONFIG_ENABLED(DEVICE_BLE)
#endif // MICROBIT_CODAL