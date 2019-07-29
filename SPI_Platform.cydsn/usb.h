#ifndef USB_H
#define USB_H

#include <stdbool.h>
#include <stdint.h>

#define USB_DEVICE (0u)
#define USB_BUFFER_SIZE (64u)

bool USB_InitHost();

void USB_Read(char* data, uint16_t* dataLength);

bool USB_ReadLine(char* data, uint16_t* dataLength);

bool USB_CanWrite();

void USB_Write(const char* data, uint16_t dataLength);

void USB_WriteLine(const char* data, uint16_t dataLength);

// void USB_WriteConst(const uint8_t * data, uint16_t dataLength);

void USB_WriteHexChar(char data);

#endif /* USB_H */
