#include <stdio.h>
#include <stdlib.h>

#define RLENC_IMPLEMENTATION
#include "rlenc.h"

static uint8_t enc_me[34] = "AAAAAAFDDCCCCCCCAEEEEEEEEEEEEEEEEE";
static uint8_t dec_me[506] = "6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17E6A1F2D7C1A17";

int main(void)
{
    rlenc_result encoded = rlenc_enc(enc_me, sizeof(enc_me), rlenc_allocer(malloc, free, 0));
    printf("encoded: %.*s\n", (int)encoded.len, encoded.data);
    rlenc_free(encoded);

    rlenc_result decoded = rlenc_dec(dec_me, sizeof(dec_me), rlenc_allocer(malloc, free, 0));
    if (decoded.status < 0) {
        fputs("ERROR: Failed to decode data\n", stderr);
        exit(EXIT_FAILURE);
    }
    printf("decoded: %.*s\n", (int)decoded.len, decoded.data);
    rlenc_free(decoded);

    return EXIT_SUCCESS;
}
