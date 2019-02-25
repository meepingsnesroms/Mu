#ifndef PDI_USB_D12_H
#define PDI_USB_D12_H

#include <stdint.h>
#include <stdbool.h>

void pdiUsbD12Reset(void);
uint32_t pdiUsbD12StateSize(void);
void pdiUsbD12SaveState(uint8_t* data);
void pdiUsbD12LoadState(uint8_t* data);

uint8_t pdiUsbD12GetRegister(bool address);
void pdiUsbD12SetRegister(bool address, uint8_t value);

#endif
