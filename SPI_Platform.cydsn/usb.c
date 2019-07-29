#include "usb.h"
#include <project.h>
#include <stdio.h>
#include <string.h>
#include "strings.h"

static bool usbState = false;

bool USB_InitHost()
{
    /* Host can send double SET_INTERFACE request. */
    if (0u != USBUART_IsConfigurationChanged())
    {
        /* Initialize IN endpoints when device is configured. */
        if (0u != USBUART_GetConfiguration())
        {
            /* Enumeration is done, enable OUT endpoint to receive data 
             * from host. */
            USBUART_CDC_Init();
            
            LED_Write(1);
            usbState = true;
        }
        else
        {
            LED_Write(0);
            usbState = false;
        }
    }
    
    return usbState;
}

void USB_Read(char* data, uint16_t* dataLength)
{
    /* Service USB CDC when device is configured. */
    if (0u != USBUART_GetConfiguration())
    {
        /* Check for input data from host. */
        if (0u != USBUART_DataIsReady())
        {
            /* Read received data and re-enable OUT endpoint. */
            *dataLength = USBUART_GetAll((uint8_t*)data);
            
            if (*dataLength > 0)
            {
                data[*dataLength] = '\0';
                *dataLength += 1;
            }
            return;
        }
    }
    *dataLength = 0;
}

bool USB_ReadLine(char* data, uint16_t* dataLength)
{
    char buffer[USB_BUFFER_SIZE];
    uint16_t bufferLength;
    uint16_t trimLength;
    uint16_t dataLengthMax = *dataLength;
    bool active = false;
    *dataLength = 0;
    
    do 
    {
        bufferLength = 0;
        trimLength = 0;
        USB_Read(buffer, &bufferLength);
        if (bufferLength > 0)
        {
            if (bufferLength + *dataLength > dataLengthMax)
            {
                *dataLength = 0;
                return false;
            }
            
            active = true;
            trimLength = strcspn(buffer, "\r\n") + 1;
            if (trimLength < bufferLength)
            {
                buffer[trimLength - 1] = '\0';
                bufferLength = trimLength;
                active = false;
            }
            
            if (*dataLength > 0)
            {
                *dataLength -= 1;
            }
            
            memcpy(&data[*dataLength], buffer, bufferLength);
            *dataLength += bufferLength;
        }
    } 
    while(active);
    
    return true;
}

bool USB_CanWrite()
{
    return USBUART_CDCIsReady() == 1;
}

void USB_Write(const char* data, uint16_t dataLength)
{
    /* Service USB CDC when device is configured. */
    if (0u != USBUART_GetConfiguration())
    {
        if (0u != dataLength)
        {
            uint16_t iterations = dataLength / USB_BUFFER_SIZE;
            
            while (iterations > 0)
            {
                /* Wait until component is ready to send data to host. */
                while (!USBUART_CDCIsReady())
                {
                }

                /* Send data back to host. */
                USBUART_PutData(((uint8_t*)data) + iterations * USB_BUFFER_SIZE, USB_BUFFER_SIZE);                
                iterations -= 1;
            }
            
            uint16_t remainingBytes = dataLength - iterations * USB_BUFFER_SIZE;
            while (!USBUART_CDCIsReady())
            {
            }
            /* If the last sent packet is exactly the maximum packet 
            *  size, it is followed by a zero-length packet to assure
            *  that the end of the segment is properly identified by 
            *  the terminal.
            */
            if (remainingBytes == 0)
            {
                /* Send zero-length packet to PC. */
                USBUART_PutData(NULL, 0u);
            }
            else
            {
                USBUART_PutData(((uint8_t*)data) + iterations * USB_BUFFER_SIZE, remainingBytes);
            }
        }
    }
}

void USB_WriteLine(const char* data, uint16_t dataLength)
{
    if (data != NULL && dataLength > 0)
    {
        USB_Write(data, dataLength);
    }
    USB_Write(STRING_NEWLINE, sizeof(STRING_NEWLINE) - 1);
}

// void USB_WriteConst(const uint8_t * data, uint16_t dataLength)
// {
//     uint8_t constBuffer[USB_BUFFER_SIZE];
//     memcpy(constBuffer, data, dataLength);
//     USB_Write(constBuffer, dataLength);
// }

void USB_WriteHexChar(char data)
{
    char buffer[3];
    sprintf(buffer, "%02X", data);
    USB_Write(buffer, 3);
}
