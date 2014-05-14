#include "utils.h"

void stripMessageId(char* source, int len, char * res) {
    int i = 0;
    for (i = 1; i < len; i++) {
        res[i - 1] = source[i];
    }
}
