#include <StringUtils.h>
void stripString(char *str) {
    while (*str != '\0') {
        if (*str == '\n') {
            *str = '\0';
            return;
        }
        ++str;
    }
}

int compareString(const char *s1, const char *s2) {
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

void Enter2Null(char *str)
{
    while (*str != '\0')
    {
        if (*str == '\n')
        {
            *str = '\0';
            return;
        }
        ++str;
    }
}

void utils_uint2str_hex(unsigned int num, char* str) {
    // num=7114 digit=4
    unsigned int temp = num;
    int digit = -1;
    *str = '0';
    *str++;
    *str = 'x';
    *str++;
    if (num == 0) {
        *str = '0';
        str++;
    }
    else {
        while (temp > 0) {
            temp /= 16;
            digit++;
        }
        for (int i = digit;i >= 0;i--) {
            int t = 1;
            for (int j = 0;j < i;j++) {
                t *= 16;
            }
            if (num / t >= 10) {
                *str = '0' + num / t + 39;
            }
            else {
                *str = '0' + num / t;
            }
            num = num % t;
            str++;
        }

    }

    *str = '\0';
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

/* align to multiple of 4  */
void Align_4(void* size){
    unsigned long* x =(unsigned long*) size;
    if((*x)&3){
        (*x) += 4-((*x)&3);
    }
}

size_t strlen(const char *s) {
  size_t i = 0;
  while (s[i]) i++;
  return i;
}