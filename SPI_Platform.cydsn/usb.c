#include "usb.h"
#include <project.h>
#include <stdio.h>

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

void USB_Read(uint8_t* data, uint16_t* dataLength)
{
    /* Service USB CDC when device is configured. */
    if (0u != USBUART_GetConfiguration())
    {
        /* Check for input data from host. */
        if (0u != USBUART_DataIsReady())
        {
            /* Read received data and re-enable OUT endpoint. */
            *dataLength = USBUART_GetAll(data);
        }
    }
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
            /* Wait until component is ready to send data to host. */
            while (0u == USBUART_CDCIsReady())
            {
            }

            /* Send data back to host. */
            USBUART_PutData((uint8_t*)data, dataLength);

            /* If the last sent packet is exactly the maximum packet 
            *  size, it is followed by a zero-length packet to assure
            *  that the end of the segment is properly identified by 
            *  the terminal.
            */
            if (USB_BUFFER_SIZE == dataLength)
            {
                /* Wait until component is ready to send data to PC. */
                while (0u == USBUART_CDCIsReady())
                {
                }

                /* Send zero-length packet to PC. */
                USBUART_PutData(NULL, 0u);
            }
        }
    }
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
    USB_Write(buffer, 2);
}
