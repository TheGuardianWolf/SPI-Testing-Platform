#ifndef USB_H
#define USB_H

#include <stdbool.h>
#include <stdint.h>

#define USB_DEVICE (0u)
#define USB_BUFFER_SIZE (64u)

bool USB_InitHost();

void USB_Read();

bool USB_CanWrite();

void USB_Write(uint8_t* data, uint16_t dataLength);

// void USB_WriteConst(const uint8_t * data, uint16_t dataLength);

void USB_WriteHexChar(uint8_t data);

#endif /* USB_H */
