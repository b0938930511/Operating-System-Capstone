#include "utils.h"
#include "mini_uart.h"

int compare_string(const char *s1, const char *s2) {
    unsigned char c1, c2;

    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0') {
            return c1 - c2;
        }
    } while (c1 == c2);

    return c1 - c2;
}

void uintoa(char *out, unsigned int i)
{   
    unsigned int index = 0;
    char tmp[20];
    do {
        unsigned char m = i % 16;
        i >>= 4;

        char c = m + '0';
        if (m == 10)
            c = 'A';
        else if (m == 11)
            c = 'B';
        else if (m == 12)
            c = 'C';
        else if (m == 13)
            c = 'D';
        else if (m == 14)
            c = 'E';
        else if (m == 15)
            c = 'F';

        tmp[index++] = c;

    } while (i);
    
    index--;
    for (unsigned int i = 0; i <= index; ++i)
        out[i] = tmp[index - i];
    out[index + 1] = '\0';
}

unsigned int getIntegerFromString(const char *str) {
    unsigned int value = 0u;

    while (*str) {
        if(*str >= '0' && *str<= '9'){
            value = value * 10u + (*str - '0');
        }
        ++str;
    }
    return value;
}

unsigned long getHexFromString(const char *str) {
    unsigned long value = 0u;

    while (*str) {
        if(*str >= '0' && *str <= '9'){
            value = value*16 + *str - '0';
        }else if(*str >= 'a' && *str <= 'z'){
            value = value*16 + *str - 'a' + 10u;
        }else if(*str >= 'A' && *str <= 'Z'){
            value = value*16 + *str - 'A' + 10u;
        }
        ++str;
    }
    return value;
}

unsigned long getHexFromString8(const char *str) {
    unsigned long ret = 0;
	for (int i = 0; i < 8; ++i) {
		if (str[i] >= '0' && str[i] <= '9')
			ret = ret * 16 + str[i] - '0';
		else if (str[i] >= 'a' && str[i] <= 'f')
			ret = ret * 16 + str[i] - 'a' + 10;
		else if (str[i] >= 'A' && str[i] <= 'F')
			ret = ret * 16 + str[i] - 'A' + 10;
	}
	return ret;
}

// convert hexadecimal string into decimal
unsigned long hexToDec(char *s) {
    unsigned long r = 0;
    for(int i = 0; i < 8; ++i) {
        if(s[i] >= '0' && s[i] <= '9')
            r = r * 16 + s[i] -'0';
        else
            r = r * 16 + s[i] - 'A' + 10;
    }
    return r;
}

/* align to multiple of 4 */
void align_4(void* size) {
    unsigned long* x =(unsigned long*) size;
    if((*x)&3){
        (*x) += 4-((*x)&3);
    }
}

/* extract the substring starts from the beginning of the src 
   and ends with the first '/' and put it into dst */
const char* slashIgnore(const char* src, char* dst, int size) {
	for (int i = 0; i < size; ++i) {
		if (src[i] == 0) {
			dst[i] = 0;
			return 0;
		} else if (src[i] == '/') {
			dst[i] = 0;
			return src + i + 1;
		} else
			dst[i] = src[i];
	}
	return 0;
}