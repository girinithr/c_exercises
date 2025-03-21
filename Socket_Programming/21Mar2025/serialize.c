#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

void packi16(unsigned char *buf, unsigned int i) {
    *buf++ = i >> 8;
    *buf++ = i;
}

void packi32(unsigned char *buf, unsigned long int i) {
    *buf++ = i >> 24;
    *buf++ = i >> 16;
    *buf++ = i >> 8;
    *buf++ = i;
}

int unpacki16(unsigned char *buf) {
    unsigned int i2 = ((unsigned int)buf[0] << 8) | buf[1];
    return (i2 <= 0x7FFF) ? i2 : -1 - (unsigned int)(0xFFFF - i2);
}

unsigned int unpacku16(unsigned char *buf) {
    return ((unsigned int)buf[0] << 8) | buf[1];
}

long int unpacki32(unsigned char *buf) {
    unsigned long int i2 = ((unsigned long int)buf[0] << 24) |
                           ((unsigned long int)buf[1] << 16) |
                           ((unsigned long int)buf[2] << 8)  |
                           buf[3];
    return (i2 <= 0x7FFFFFFF) ? i2 : -1 - (long int)(0xFFFFFFFF - i2);
}

unsigned long int unpacku32(unsigned char *buf) {
    return ((unsigned long int)buf[0] << 24) |
           ((unsigned long int)buf[1] << 16) |
           ((unsigned long int)buf[2] << 8)  |
           buf[3];
}

unsigned int pack(unsigned char *buf, char *format, ...) {
    va_list ap;
    int h;
    unsigned int H;
    long int l;
    char *s;
    unsigned int len, size = 0;

    va_start(ap, format);
    for (; *format != '\0'; format++) {
        switch (*format) {
        case 'c':
            *buf++ = (signed char)va_arg(ap, int);
            size += 1;
            break;
        case 'h':
            h = va_arg(ap, int);
            packi16(buf, h);
            buf += 2;
            size += 2;
            break;
        case 'H':
            H = va_arg(ap, unsigned int);
            packi16(buf, H);
            buf += 2;
            size += 2;
            break;
        case 'l':
            l = va_arg(ap, long int);
            packi32(buf, l);
            buf += 4;
            size += 4;
            break;
        case 's':
            s = va_arg(ap, char*);
            len = strlen(s);
            packi16(buf, len);
            buf += 2;
            memcpy(buf, s, len);
            buf += len;
            size += len + 2;
            break;
        }
    }
    va_end(ap);
    return size;
}

void unpack(unsigned char *buf, char *format, ...) {
    va_list ap;
    int *h;
    unsigned int *H;
    long int *l;
    char *s;
    unsigned int len;

    va_start(ap, format);
    for (; *format != '\0'; format++) {
        switch (*format) {
        case 'c':
            *va_arg(ap, signed char*) = *buf++;
            break;
        case 'h':
            h = va_arg(ap, int*);
            *h = unpacki16(buf);
            buf += 2;
            break;
        case 'H':
            H = va_arg(ap, unsigned int*);
            *H = unpacku16(buf);
            buf += 2;
            break;
        case 'l':
            l = va_arg(ap, long int*);
            *l = unpacki32(buf);
            buf += 4;
            break;
        case 's':
            s = va_arg(ap, char*);
            len = unpacku16(buf);
            buf += 2;
            memcpy(s, buf, len);
            s[len] = '\0';
            buf += len;
            break;
        }
    }
    va_end(ap);
}

int main(void) {
    unsigned char buf[1024];
    int8_t magic;
    int16_t monkeycount;
    int32_t altitude;
    char *s = "Great unmitigated Zot! You've found the Runestaff!";
    char s2[96];
    int16_t packetsize, ps2;

    packetsize = pack(buf, "chhls", (int8_t)'B', (int16_t)0, (int16_t)37, (int32_t)-5, s);
    packi16(buf+1, packetsize);

    printf("Packet size: %d bytes\n", packetsize);

    unpack(buf, "chhl96s", &magic, &ps2, &monkeycount, &altitude, s2);
    printf("Unpacked Data:\n");
    printf("Magic: '%c'\n", magic);
    printf("Packet Size: %d\n", ps2);
    printf("Monkey Count: %d\n", monkeycount);
    printf("Altitude: %d\n", altitude);
    printf("String: %s\n", s2);
    return 0;
}
