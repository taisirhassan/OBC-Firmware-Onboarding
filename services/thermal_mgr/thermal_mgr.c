#include "thermal_mgr.h"
#include "errors.h"
#include "lm75bd.h"
#include "console.h"

#include <FreeRTOS.h>
#include <os_task.h>
#include <os_queue.h>

#include <string.h>

#define THERMAL_MGR_STACK_SIZE 256U

static TaskHandle_t thermalMgrTaskHandle;
static StaticTask_t thermalMgrTaskBuffer;
static StackType_t thermalMgrTaskStack[THERMAL_MGR_STACK_SIZE];

#define THERMAL_MGR_QUEUE_LENGTH 10U
#define THERMAL_MGR_QUEUE_ITEM_SIZE sizeof(thermal_mgr_event_t)

static QueueHandle_t thermalMgrQueueHandle;
static StaticQueue_t thermalMgrQueueBuffer;
static uint8_t thermalMgrQueueStorageArea[THERMAL_MGR_QUEUE_LENGTH * THERMAL_MGR_QUEUE_ITEM_SIZE];

static void thermalMgr(void *pvParameters);

void initThermalSystemManager(lm75bd_config_t *config) {
  memset(&thermalMgrTaskBuffer, 0, sizeof(thermalMgrTaskBuffer));
  memset(thermalMgrTaskStack, 0, sizeof(thermalMgrTaskStack));
  
  thermalMgrTaskHandle = xTaskCreateStatic(
    thermalMgr, "thermalMgr", THERMAL_MGR_STACK_SIZE,
    config, 1, thermalMgrTaskStack, &thermalMgrTaskBuffer);

  memset(&thermalMgrQueueBuffer, 0, sizeof(thermalMgrQueueBuffer));
  memset(thermalMgrQueueStorageArea, 0, sizeof(thermalMgrQueueStorageArea));

  thermalMgrQueueHandle = xQueueCreateStatic(
    THERMAL_MGR_QUEUE_LENGTH, THERMAL_MGR_QUEUE_ITEM_SIZE,
    thermalMgrQueueStorageArea, &thermalMgrQueueBuffer);

}

error_code_t thermalMgrSendEvent(thermal_mgr_event_t *event) {
  /* Send an event to the thermal manager queue */
  // Check if the event is NULL
  if(event == NULL) {
    return ERR_CODE_INVALID_ARG;
  }

  // Send the event to the queue. If the queue is full, return an error code.
if(xQueueSend(thermalMgrQueueHandle, event, 0) != pdTRUE) {
    return ERR_CODE_QUEUE_FULL;
  }

  return ERR_CODE_SUCCESS;
}

void osHandlerLM75BD(void) {
  /* Implement this function */
  // Create an event variable to store the OS interrupt event
  thermal_mgr_event_t interrupt = {
    .type = THERMAL_MGR_EVENT_OS_INTERRUPT
  };
  
  // Send the OS interrupt event to the thermal manager queue
  thermalMgrSendEvent(&interrupt);
}

static void thermalMgr(void *pvParameters) {
  /* Implement this task */
    float tempC;
  while (1) {
  // Create an event variable to store incoming events from the queue
  thermal_mgr_event_t event;
  // Cast the pvParameters to the configuration struct
lm75bd_config_t *config = (lm75bd_config_t *)pvParameters;
 if(xQueueReceive(thermalMgrQueueHandle, &event, portMAX_DELAY) == pdTRUE) {
  // The type of the received event is checked.
    switch (event.type) {
      // If the event type is THERMAL_MGR_EVENT_MEASURE_TEMP_CMD, the task reads the current temperature from the LM75BD sensor.
      case THERMAL_MGR_EVENT_MEASURE_TEMP_CMD: {
        // Read the temperature from the LM75BD sensor
        if (readTempLM75BD(config->devAddr, &tempC) == ERR_CODE_SUCCESS) {
          addTemperatureTelemetry(tempC);
        }
        break;
      }
      case THERMAL_MGR_EVENT_OS_INTERRUPT: {
        // If the event type is THERMAL_MGR_EVENT_OS_INTERRUPT, the task checks if the hysteresis condition is met.
       if (tempC > config->hysteresisThresholdCelsius) {
          overTemperatureDetected();
        } else {
          safeOperatingConditions();
        }
        break;
      }
        // If the event type is not recognized, the task does nothing.
      default:
        break;
    }
  }
  }
}
   

void addTemperatureTelemetry(float tempC) {
  printConsole("Temperature telemetry: %f deg C\n", tempC);
}

void overTemperatureDetected(void) {
  printConsole("Over temperature detected!\n");
}

void safeOperatingConditions(void) { 
  printConsole("Returned to safe operating conditions!\n");
}
