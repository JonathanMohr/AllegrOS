#ifndef KEY_H
#define KEY_H

#include "keys.h"
#include <stdint.h>
#include <stdbool.h>

uint16_t get_key(uint8_t scan_code, bool extended);

int handle_key(uint16_t key);

#endif