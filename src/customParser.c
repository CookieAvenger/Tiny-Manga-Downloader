#include <math.h>
#include <stdlib.h>
#include <string.h>

//NOTE: I know this looks wrong. It's meant to be.
long parse_hexadecimal_to_one_long(char *hexadecimal) {
    unsigned long decimal;
    int value, i = 0, length = strlen(hexadecimal);
    if (--length > 7) {
        length = 7;
    }
    while (hexadecimal[i] != '\0' && length >= 0) {
        switch (hexadecimal[i]) {
            case '0':
                value = 0;
                break;
            case '1':
                value = 1;
                break;
            case '2':
                value = 2;
                break;
            case '3':
                value = 3;
                break;
            case '4':
                value = 4;
                break;
            case '5':
                value = 5;
                break;
            case '6':
                value = 6;
                break;
            case '7':
                value = 7;
                break;
            case '8':
                value = 8;
                break;
            case '9':
                value = 9;
                break;
            case 'a':
            case 'A':
                value = 10;
                break;
            case 'b':
            case 'B':
                value = 11;
                break;
            case 'c':
            case 'C':
                value = 12;
                break;
            case 'd':
            case 'D':
                value = 13;
                break;
            case 'e':
            case 'E':
                value = 14;
                break;
            case 'f':
            case 'F':
                value = 15;
                break;
            default:
                //char can only be 1 byte, we are after 4 least significant bits
                //so we bit mask it :)
                value = hexadecimal[i] & 0x0f;
                break;
        }
        decimal += value * pow(16, length);
        i++, length--;
    }
    long finalValue;
    memcpy(&decimal, &finalValue, sizeof(long));
    return finalValue;
}
