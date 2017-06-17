#include "md5.h"
#include <string.h>
#include <stdio.h>

int main()
{
    const char* str1 = "123";
    const char* str2 = "456";
    const char* str3 = NULL;

    printf("%s: %s\n", str1, QMD5_CString(str1, strlen(str1)).c_str());
    printf("%s: %s (string, enhanced)\n", str1, QMD5_String(str1, true).c_str());

    printf("%s (enhanced): %s\n", str1, QMD5_CString(str1, strlen(str1), true).c_str());
    printf("%s: %s (size = 0)\n", str1, QMD5_CString(str1, 0).c_str());
    printf("%s: %s\n", str2, QMD5_CString(str2, strlen(str2)).c_str());
    auto str = QMD5_CString(str3, 0);
    printf("%s: %s\n", str3, str.c_str());

    printf("md5.cc: %s\n", QMD5_File("md5.cc").c_str());
    printf("md5.cc (enhanced): %s\n", QMD5_File("md5.cc", true).c_str());
    printf("invalid file: %s\n", QMD5_File("123.c").c_str());
    printf("null file: %s\n", QMD5_File(NULL).c_str());
}