#include "spi.h"
#include <project.h>


bool SPI_TxReady()
{
    return (SPIM_ReadTxStatus() & 0b00010001) > 0;
}
