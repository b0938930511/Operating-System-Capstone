#ifndef	_UtilS_H
#define	_UtilS_H

#define MAX_BUFFER_SIZE 256u

int compare_string(const char *s1, const char *s2);
void uintoa(char *out, unsigned int i);
unsigned int getIntegerFromString(const char *str);
unsigned long getHexFromString(const char *str);
unsigned long getHexFromString8(const char *str);
unsigned long hexToDec(char *s);
void align_4(void* size);
const char* slashIgnore(const char* src, char* dst, int size);

#endif