#ifndef __STRING_UTILS_H
#define __STRING_UTILS_H

void stripString(char *str);
int compareString(const char *s1, const char *s2);
void utils_uint2str_hex(unsigned int num, char* str) ;
unsigned int getIntegerFromString(const char *str);
unsigned long getHexFromString(const char *str);
void Enter2Null(char *str);

#endif
