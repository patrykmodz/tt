#include <stdlib.h>
#include "prompt.h"

static char *prompt_buffer;
static size_t prompt_size;
static size_t prompt_capacity;
