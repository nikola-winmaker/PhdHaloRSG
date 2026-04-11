#include "classical_api.h"

void classical_hello(void) {
    uart_write_string( "\n[APP2] Starting FreeRTOS scheduler Classical\n");
}
