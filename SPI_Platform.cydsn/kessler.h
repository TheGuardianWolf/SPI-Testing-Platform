/*
 * kessler.h
 *
 *  Created on: 16/07/2019
 *      Author: Jerry Fan
 */

#ifndef CORE_KESSLER_H_
#define CORE_KESSLER_H_

#include <stdint.h>

// Kessler SPI Protocol
#define KSPI_START_BYTE (0xD8)
#define KSPI_QUERY_HEADER_SIZE (3) // Start byte is not header
#define KSPI_RESPONSE_HEADER_SIZE (2)
#define KSPI_DATA_MAX_SIZE (255)

typedef enum {
    KSPI_RESPONSECODE_TODO = 0
} KSPI_ResponseCode_t;

typedef enum {
    KSPI_QUERYCODE_TODO = 0
} KSPI_QueryCode_t;

typedef enum {
    KSPI_QUERYSUBCODE_TODO = 0
} KSPI_QuerySubcode_t;

typedef struct {
    KSPI_QueryCode_t queryCode;
    KSPI_QuerySubcode_t querySubcode;
    uint8_t length;
    uint8_t* data;
} KSPI_Query_t;

typedef struct {
    KSPI_ResponseCode_t responseCode;
    uint8_t length;
    uint8_t* data;
} KSPI_Response_t;


#endif /* CORE_KESSLER_H_ */
