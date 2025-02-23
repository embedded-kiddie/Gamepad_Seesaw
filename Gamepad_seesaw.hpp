/*!
 * @file Gamepad_seesaw.hpp
 *
 * This is a modified version of Adafruit_seesaw.h and Adafruit_seesaw.cpp
 * written by Dean Miller for Adafruit Industries.
 *
 * Adafruit's seesaw driver is part of for the Arduino platform.  It is
 * designed specifically to work with the Adafruit products that use seesaw
 * technology.
 *
 * These chips use I2C to communicate, 2 pins (SCL+SDA) are required
 * to interface with the board.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */

#ifndef _GAMEPAD_SEESAW_HPP_
#define _GAMEPAD_SEESAW_HPP_
 
#include "Adafruit_I2CDevice.h"
#include <Arduino.h>
#include <Wire.h>

//#define DEBUG_SERIAL

#define SEESAW_GAMEPAD_ADDRESS    0x50
#define SEESAW_STATUS_BASE        0x00
#define SEESAW_STATUS_HW_ID       0x01
#define SEESAW_STATUS_VERSION     0x02
#define SEESAW_STATUS_SWRST       0x7F
#define SEESAW_GPIO_BASE          0x01
#define SEESAW_GPIO_BULK          0x04
#define SEESAW_GPIO_DIRSET_BULK   0x02
#define SEESAW_GPIO_DIRCLR_BULK   0x03
#define SEESAW_GPIO_BULK_SET      0x05
#define SEESAW_GPIO_BULK_CLR      0x06
#define SEESAW_GPIO_INTENSET      0x08
#define SEESAW_GPIO_INTENCLR      0x09
#define SEESAW_GPIO_PULLENSET     0x0B
#define SEESAW_ADC_BASE           0x09
#define SEESAW_ADC_CHANNEL_OFFSET 0x07

#define SEESAW_HW_ID_CODE_SAMD09    0x55 ///< seesaw HW ID code for SAMD09
#define SEESAW_HW_ID_CODE_TINY806   0x84 ///< seesaw HW ID code for ATtiny806
#define SEESAW_HW_ID_CODE_TINY807   0x85 ///< seesaw HW ID code for ATtiny807
#define SEESAW_HW_ID_CODE_TINY816   0x86 ///< seesaw HW ID code for ATtiny816
#define SEESAW_HW_ID_CODE_TINY817   0x87 ///< seesaw HW ID code for ATtiny817
#define SEESAW_HW_ID_CODE_TINY1616  0x88 ///< seesaw HW ID code for ATtiny1616
#define SEESAW_HW_ID_CODE_TINY1617  0x89 ///< seesaw HW ID code for ATtiny1617

class Gamepad_seesaw {
protected:
  TwoWire *_i2cbus;                     /*!< The I2C Bus used to communicate with the seesaw */
  Adafruit_I2CDevice *_i2c_dev = NULL;  ///< The BusIO device for I2C control

public:
  // constructors
  Gamepad_seesaw(TwoWire *i2c_bus = nullptr) {
    if (i2c_bus == NULL) {
      _i2cbus = &Wire;
    } else {
      _i2cbus = i2c_bus;
    }
  }
  ~Gamepad_seesaw(void){};

  bool write(uint8_t regHigh, uint8_t regLow, uint8_t *buf = NULL, uint8_t num = 0) {
    uint8_t prefix[2] = { regHigh, regLow };

    if (!_i2c_dev->write(buf, num, true, prefix, 2)) {
      return false;
    }

    return true;
  }

  bool write8(byte regHigh, byte regLow, byte value) {
    return write(regHigh, regLow, &value, 1);
  }

  bool read(uint8_t regHigh, uint8_t regLow, uint8_t *buf, uint8_t num, uint16_t delay = 250) {
    uint8_t pos = 0;
    uint8_t prefix[2] = { regHigh, regLow };

    // on arduino we need to read in 32 byte chunks
    while (pos < num) {
      uint8_t read_now = min(32, num - pos);

      if (!_i2c_dev->write(prefix, 2)) {
        return false;
      }

      // TODO: tune this
      //delayMicroseconds(delay);

#ifdef DEBUG_SERIAL
      Serial.print("Reading ");
      Serial.print(read_now);
      Serial.println(" bytes");
#endif

      if (!_i2c_dev->read(buf + pos, read_now)) {
        return false;
      }
      pos += read_now;

#ifdef DEBUG_SERIAL
      Serial.print("pos: ");
      Serial.print(pos);
      Serial.print(" num:");
      Serial.println(num);
#endif
    }

    return true;
  }

  bool SWReset() {
    return write8(SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, 0xFF);
  }

  bool begin(uint8_t addr = SEESAW_GAMEPAD_ADDRESS, uint8_t sda = SDA, uint8_t scl = SCL, uint32_t frequency = 400000) {
    if (_i2c_dev) {
      delete _i2c_dev;
    }

    _i2cbus->begin(sda, scl);
    _i2c_dev = new Adafruit_I2CDevice(addr, _i2cbus);

    bool found = false;
    for (int retries = 0; retries < 10; retries++) {
      if (_i2c_dev->begin()) {
        found = true;
        break;
      }
      delay(10);
    }

    if (!found) {
      Serial.println("No found 0");
      return false;
    }

#ifdef DEBUG_SERIAL
    Serial.println("Begun");
#endif

    if (true/*reset*/) {
      found = false;
      SWReset();
      for (int retries = 0; retries < 10; retries++) {
        if (_i2c_dev->detected()) {
          found = true;
          break;
        }
        delay(10);
      }
    }

    if (!found) {
      Serial.println("No found 1");
      return false;
    }

#ifdef DEBUG_SERIAL
    Serial.println("Reset");
#endif

    found = false;
    for (int retries = 0; !found && retries < 10; retries++) {
      uint8_t c = 0;

      read(SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID, &c, 1);
      if ((c == SEESAW_HW_ID_CODE_SAMD09   ) ||
          (c == SEESAW_HW_ID_CODE_TINY817  ) ||
          (c == SEESAW_HW_ID_CODE_TINY807  ) ||
          (c == SEESAW_HW_ID_CODE_TINY816  ) ||
          (c == SEESAW_HW_ID_CODE_TINY806  ) ||
          (c == SEESAW_HW_ID_CODE_TINY1616 ) ||
          (c == SEESAW_HW_ID_CODE_TINY1617)) {
        found = true;
      }

      delay(10);
    }

#ifdef DEBUG_SERIAL
    Serial.println("Done!");
#endif

    _i2cbus->setClock(frequency);
    return found;
  }

  uint16_t analogRead(uint8_t pin) {
    uint8_t buf[2];
    read(SEESAW_ADC_BASE, SEESAW_ADC_CHANNEL_OFFSET + pin, buf, 2, 500);
    uint16_t ret = ((uint16_t)buf[0] << 8) | buf[1];
    //delay(1);
    return ret;
  }

  uint32_t digitalReadBulk(uint32_t pins) {
    uint8_t buf[4];
    read(SEESAW_GPIO_BASE, SEESAW_GPIO_BULK, buf, 4);
    uint32_t ret = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
    return ret & pins;
  }

  uint32_t getVersion() {
    uint8_t buf[4];
    read(SEESAW_STATUS_BASE, SEESAW_STATUS_VERSION, buf, 4);
    uint32_t ret = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
    return ret;
  }

  void pinModeBulk(uint32_t pins, uint8_t mode) {
    uint8_t cmd[] = { (uint8_t)(pins >> 24), (uint8_t)(pins >> 16),
                      (uint8_t)(pins >>  8), (uint8_t)(pins) };
    switch (mode) {
      case OUTPUT:
        write(SEESAW_GPIO_BASE, SEESAW_GPIO_DIRSET_BULK, cmd, 4);
        break;
      case INPUT:
        write(SEESAW_GPIO_BASE, SEESAW_GPIO_DIRCLR_BULK, cmd, 4);
        break;
      case INPUT_PULLUP:
        write(SEESAW_GPIO_BASE, SEESAW_GPIO_DIRCLR_BULK, cmd, 4);
        write(SEESAW_GPIO_BASE, SEESAW_GPIO_PULLENSET, cmd, 4);
        write(SEESAW_GPIO_BASE, SEESAW_GPIO_BULK_SET, cmd, 4);
        break;
      case INPUT_PULLDOWN:
        write(SEESAW_GPIO_BASE, SEESAW_GPIO_DIRCLR_BULK, cmd, 4);
        write(SEESAW_GPIO_BASE, SEESAW_GPIO_PULLENSET, cmd, 4);
        write(SEESAW_GPIO_BASE, SEESAW_GPIO_BULK_CLR, cmd, 4);
        break;
    }
  }

  void setGPIOInterrupts(uint32_t pins, bool enabled) {
    uint8_t cmd[] = { (uint8_t)(pins >> 24), (uint8_t)(pins >> 16),
                      (uint8_t)(pins >>  8), (uint8_t)(pins) };
    if (enabled) {
      write(SEESAW_GPIO_BASE, SEESAW_GPIO_INTENSET, cmd, 4);
    } else {
      write(SEESAW_GPIO_BASE, SEESAW_GPIO_INTENCLR, cmd, 4);
    }
  }
};

#endif  // _GAMEPAD_SEESAW_HPP_
