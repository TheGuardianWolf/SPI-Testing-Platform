#include <project.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "kessler.h"
#include "util.h"
#include "usb.h"
#include "strings.h"
#include "spi.h"

#if defined (__GNUC__)
    /* Add an explicit reference to the floating point printf library */
    /* to allow usage of the floating point conversion specifiers. */
    /* This is not linked in by default with the newlib-nano library. */
    asm (".global _printf_float");
#endif

/* The buffer size is equal to the maximum packet size of the IN and OUT bulk
* endpoints.
*/
#define SPI_FILL_BYTE (0xFF)

typedef enum 
{
    STATE_IDLE,
    STATE_GET_QUERY_CODE,
    STATE_GET_QUERY_SUBCODE,
    STATE_GET_QUERY_LENGTH,
    STATE_GET_QUERY_DATA,
    STATE_SEND_QUERY,
    STATE_RECV_START,
    STATE_RECV_HEADER,
    STATE_RECV_DATA
}
State_t;

static State_t state = STATE_IDLE;

static uint8_t queryData[KSPI_DATA_MAX_SIZE];
static KSPI_Query_t query = {
    .queryCode = KSPI_QUERYCODE_TODO,
    .querySubcode = KSPI_QUERYSUBCODE_TODO,
    .length = 0,
    .data = queryData
};

static uint8_t responseData[KSPI_DATA_MAX_SIZE];
static KSPI_Response_t response = {
    .responseCode = KSPI_RESPONSECODE_TODO,
    .length = 0,
    .data = responseData
};

static uint16_t usbRxBufferLength = 0;
static uint8_t usbRxBuffer[USB_BUFFER_SIZE];

int main()
{    
    CyGlobalIntEnable;

    /* Start USBFS operation with 5-V operation. */
    USBUART_Start(USB_DEVICE, USBUART_5V_OPERATION);
    SPIM_Start();
    
    for(;;)
    {
        bool initSuccess = USB_InitHost();

        if (initSuccess)
        {   
            USB_Read(usbRxBuffer, &usbRxBufferLength);
            State_t nextState = STATE_IDLE;
            
            switch(state)
            {
                case STATE_IDLE:
                if (USB_CanWrite())
                {
                    USB_Write(STRING_QUERY_CODE, sizeof(STRING_QUERY_CODE));
                    nextState = STATE_GET_QUERY_CODE;
                }
                else
                {
                    nextState = STATE_IDLE;
                }
                break;
                case STATE_GET_QUERY_CODE:
                if (usbRxBufferLength > 0)
                {
                    int queryCode;
                    str2int_errno result = str2int(&queryCode, (char*)usbRxBuffer, 10);
                    if (result == STR2INT_SUCCESS && queryCode == 0)
                    {
                        query.queryCode = (KSPI_QueryCode_t)queryCode;
                        USB_Write(STRING_QUERY_SUBCODE, sizeof(STRING_QUERY_SUBCODE));
                        nextState = STATE_GET_QUERY_SUBCODE;
                    }
                    else
                    {
                        nextState = STATE_IDLE;
                    }
                }
                else
                {
                    nextState = STATE_GET_QUERY_CODE;
                }
                break;
                case STATE_GET_QUERY_SUBCODE:
                if (usbRxBufferLength > 0)
                {
                    int querySubcode;
                    str2int_errno result = str2int(&querySubcode, (char*)usbRxBuffer, 10);
                    if (result == STR2INT_SUCCESS && querySubcode == 0)
                    {
                        query.querySubcode = (KSPI_QuerySubcode_t)querySubcode;
                        USB_Write(STRING_QUERY_LENGTH, sizeof(STRING_QUERY_LENGTH));
                        nextState = STATE_GET_QUERY_LENGTH;
                    }
                    else
                    {
                        USB_Write(STRING_QUERY_SUBCODE, sizeof(STRING_QUERY_SUBCODE));
                        nextState = STATE_GET_QUERY_SUBCODE;
                    }
                }
                else
                {
                    nextState = STATE_GET_QUERY_SUBCODE;
                }
                break;
                case STATE_GET_QUERY_LENGTH:
                if (usbRxBufferLength > 0)
                {
                    int length;
                    str2int_errno result = str2int(&length, (char*)usbRxBuffer, 10);
                    if (result == STR2INT_SUCCESS)
                    {
                        query.length = (KSPI_QueryCode_t)length;

                        if (length == 0)
                        {
                            USB_Write(STRING_SENDING_QUERY, sizeof(STRING_SENDING_QUERY));
                            nextState = STATE_SEND_QUERY;
                        }
                        else
                        {
                            USB_Write(STRING_QUERY_DATA, sizeof(STRING_QUERY_DATA));
                            nextState = STATE_GET_QUERY_DATA;
                        }
                    }
                    else
                    {
                        USB_Write(STRING_QUERY_LENGTH, sizeof(STRING_QUERY_LENGTH));
                        nextState = STATE_GET_QUERY_LENGTH;
                    }
                }
                else
                {
                    nextState = STATE_GET_QUERY_LENGTH;
                }
                break;
                case STATE_GET_QUERY_DATA:
                if (usbRxBufferLength > 0)
                {
                    memcpy(query.data, usbRxBuffer, usbRxBufferLength);
                    USB_Write(STRING_SENDING_QUERY, sizeof(STRING_SENDING_QUERY));
                    nextState = STATE_SEND_QUERY;
                }
                else
                {
                    nextState = STATE_GET_QUERY_DATA;
                }
                break;
                case STATE_SEND_QUERY:
                if (SPI_TxReady());
                {
                    SPIM_WriteTxData(KSPI_START_BYTE);
                    SPIM_WriteTxData((uint8_t)query.queryCode);
                    SPIM_WriteTxData((uint8_t)query.querySubcode);
                    SPIM_WriteTxData(query.length);
                    SPIM_PutArray(query.data, query.length);

                    USB_Write(STRING_TX, sizeof(STRING_TX));
                    
                    USB_WriteHexChar(KSPI_START_BYTE);
                    USB_WriteHexChar((uint8_t)query.queryCode);
                    USB_WriteHexChar((uint8_t)query.querySubcode);
                    USB_WriteHexChar(query.length);
                    for (uint8_t i; i < query.length; i += 1)
                    {
                        SB_WriteHexChar(query.data[i]);
                    }
                    USB_Write(STRING_NEWLINE, sizeof(STRING_NEWLINE));

                    USB_Write(STRING_RX, sizeof(STRING_RX));

                    while (!SPI_TxReady());

                    uint8_t byte;
                    while(SPIM_GetRxBufferSize() > 0)
                    {
                        byte = SPIM_ReadRxData();
                        USB_WriteHexChar(byte);
                    }

                    USB_Write(STRING_NEWLINE, sizeof(STRING_NEWLINE));

                    USB_Write(STRING_RECEIVE_WAIT, sizeof(STRING_RECEIVE_WAIT));

                    nextState = STATE_RECV_START;
                }
                else
                {
                    nextState = STATE_SEND_QUERY;
                }
                break;
                case STATE_RECV_START:
                {
                    SPIM_ClearFIFO();
                    SPIM_WriteByte(SPI_FILL_BYTE);

                    USB_Write(STRING_TX, sizeof(STRING_TX));
                    USB_WriteHexChar(SPI_FILL_BYTE);

                    while (!SPI_TxReady());

                    uint8_t byte = SPIM_ReadRxData();

                    USB_Write(STRING_RX, sizeof(STRING_RX));
                    USB_WriteHexChar(byte);
                    USB_Write(STRING_NEWLINE, sizeof(STRING_NEWLINE));

                    if (byte == KSPI_START_BYTE)
                    {
                        USB_Write(STRING_RECEIVE_HEADER, sizeof(STRING_RECEIVE_HEADER));
                        nextState = STATE_RECV_HEADER;
                    }
                    else
                    {
                        nextState = STATE_RECV_START;
                    }
                }
                break;
                case STATE_RECV_HEADER:
                {
                    uint8_t buffer[2] = { SPI_FILL_BYTE, SPI_FILL_BYTE };
                    SPIM_PutArray(buffer, 2);

                    USB_Write(STRING_TX, sizeof(STRING_TX));
                    USB_WriteHexChar(SPI_FILL_BYTE);
                    USB_WriteHexChar(SPI_FILL_BYTE);
                    USB_Write(STRING_NEWLINE, sizeof(STRING_NEWLINE));

                    while (!SPI_TxReady());

                    response.responseCode = (KSPI_ResponseCode_t)SPIM_ReadRxData();
                    response.length = SPIM_ReadRxData();

                    USB_Write(STRING_RX, sizeof(STRING_RX));
                    USB_WriteHexChar((uint8_t)response.responseCode);
                    USB_WriteHexChar(response.length);
                    USB_Write(STRING_NEWLINE, sizeof(STRING_NEWLINE));

                    if (response.length > 0)
                    {
                        USB_Write(STRING_RECEIVE_DATA, sizeof(STRING_RECEIVE_DATA));
                        nextState = STATE_RECV_DATA;
                    }
                    else
                    {
                        nextState = STATE_IDLE;
                    }
                }
                break;
                case STATE_RECV_DATA:
                {
                    USB_Write(STRING_RECEIVE_DATA, sizeof(STRING_RECEIVE_DATA));

                    uint8_t buffer[KSPI_DATA_MAX_SIZE];
                    memset(buffer, SPI_FILL_BYTE, response.length);
                    SPIM_PutArray(buffer, response.length);

                    USB_Write(STRING_TX, sizeof(STRING_TX));
                    for (uint8_t i = 0; i < response.length; i += 1)
                    {
                        USB_WriteHexChar(SPI_FILL_BYTE);
                    }
                    USB_Write(STRING_NEWLINE, sizeof(STRING_NEWLINE));

                    while (!SPI_TxReady());

                    USB_Write(STRING_RX, sizeof(STRING_RX));

                    uint8_t byte;
                    while(SPIM_GetRxBufferSize() > 0)
                    {
                        byte = SPIM_ReadRxData();
                        USB_WriteHexChar(byte);
                    }
                    USB_Write(STRING_NEWLINE, sizeof(STRING_NEWLINE));
                    
                    nextState = STATE_IDLE;
                }
                break;
                default:
                break;
            }
            
            state = nextState;
        }
    }
}


/* [] END OF FILE */
