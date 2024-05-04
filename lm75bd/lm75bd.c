#include "lm75bd.h"
#include "i2c_io.h"
#include "errors.h"
#include "logging.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

/* LM75BD Registers (p.8) */
#define LM75BD_REG_TEMP 0x00U  /* Temperature Register (R) */
#define LM75BD_REG_CONF 0x01U  /* Configuration Register (R/W) */

error_code_t lm75bdInit(lm75bd_config_t *config) {
  error_code_t errCode;

  if (config == NULL) return ERR_CODE_INVALID_ARG;

  RETURN_IF_ERROR_CODE(writeConfigLM75BD(config->devAddr, config->osFaultQueueSize, config->osPolarity,
                                         config->osOperationMode, config->devOperationMode));

  // Assume that the overtemperature and hysteresis thresholds are already set
  // Hysteresis: 75 degrees Celsius
  // Overtemperature: 80 degrees Celsius

  return ERR_CODE_SUCCESS;
}

error_code_t readTempLM75BD(uint8_t devAddr, float *temp) {
  /* Implement this driver function */
  error_code_t errCode;

  // Check if the temperature pointer is NULL
if (temp == NULL) return ERR_CODE_INVALID_ARG;

uint8_t tempRegData = LM75BD_REG_TEMP;

// Send the address of the temperature register
RETURN_IF_ERROR_CODE(i2cSendTo(devAddr, &tempRegData, 1));

// Read the temperature data
uint8_t tempData[2] = {0};
RETURN_IF_ERROR_CODE(i2cReceiveFrom(devAddr, tempData, 2));

// Combine the two bytes into a single 16-bit value
uint16_t tempRaw = (tempData[0] << 8) | tempData[1];

// Shift the value to the right by 5 bits to get the 11 bits of temperature data
tempRaw >>= 5;

if (tempRaw & 0x400) {
  // If D10 bit is set, the temperature is negative and must be converted into two's complement
  tempRaw = (~tempRaw & 0x7FF) + 1; // Mask the 11 bits and add 1 to get the two's complement
  *temp = -tempRaw * 0.125f;
} else {
  // If D10 bit is not set, the temperature is positive
  *temp = tempRaw * 0.125f;


}
  return ERR_CODE_SUCCESS;
}

#define CONF_WRITE_BUFF_SIZE 2U
error_code_t writeConfigLM75BD(uint8_t devAddr, uint8_t osFaultQueueSize, uint8_t osPolarity,
                                   uint8_t osOperationMode, uint8_t devOperationMode) {
  error_code_t errCode;

  // Stores the register address and data to be written
  // 0: Register address
  // 1: Data
  uint8_t buff[CONF_WRITE_BUFF_SIZE] = {0};

  buff[0] = LM75BD_REG_CONF;

  uint8_t osFaltQueueRegData = 0;
  switch (osFaultQueueSize) {
    case 1:
      osFaltQueueRegData = 0;
      break;
    case 2:
      osFaltQueueRegData = 1;
      break;
    case 4:
      osFaltQueueRegData = 2;
      break;
    case 6:
      osFaltQueueRegData = 3;
      break;
    default:
      return ERR_CODE_INVALID_ARG;
  }

  buff[1] |= (osFaltQueueRegData << 3);
  buff[1] |= (osPolarity << 2);
  buff[1] |= (osOperationMode << 1);
  buff[1] |= devOperationMode;

  errCode = i2cSendTo(LM75BD_OBC_I2C_ADDR, buff, CONF_WRITE_BUFF_SIZE);
  if (errCode != ERR_CODE_SUCCESS) return errCode;

  return ERR_CODE_SUCCESS;
}
