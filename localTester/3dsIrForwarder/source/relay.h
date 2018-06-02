#pragma once

#include <stdint.h>
#include <stdbool.h>

enum{
   RELAY_ERROR_NONE = 0,
   RELAY_ERROR_NOT_INITED,
   RELAY_ERROR_INVALID_PARAM,
   RELAY_ERROR_OUT_OF_MEMORY,
   RELAY_ERROR_TIMEOUT,
   RELAY_ERROR_WIFI_DRIVER,
   RELAY_ERROR_IR_DRIVER,
   RELAY_ERROR_CANT_OPEN_SOCKET,
   RELAY_ERROR_CANT_BIND_SOCKET,
   RELAY_ERROR_CANT_LISTEN_SOCKET,
   RELAY_ERROR_NO_CLIENTS
};

uint32_t relayInit(uint32_t irBaud);
void relayExit();
uint32_t relayAttemptConnection();

uint32_t relayGetMyIp();
void relayRun();
