#pragma once

#include <stddef.h>
#include <string>

#define QMD5_DIGEST_LENGTH 16
#define QMD5_LONG unsigned long
#define QMD5_CBLOCK 64
#define QMD5_LBLOCK (QMD5_CBLOCK / 4)
#define QMD5SUM_NULL "d41d8cd98f00b204e9800998ecf8427e"

typedef struct QMD5state_st
{
    QMD5_LONG A, B, C, D;
    QMD5_LONG Nl, Nh;
    QMD5_LONG data[QMD5_LBLOCK];
    unsigned int num;
} QMD5_CTX;

int QMD5_Init(QMD5_CTX *c);
int QMD5_Update(QMD5_CTX *c, const void *data_, size_t len);
int QMD5_Final(unsigned char *md, QMD5_CTX *c);
unsigned char *QMD5(const unsigned char *d, size_t n, unsigned char *md);

std::string QMD5_File(const char *filename, bool enhanced = false);
std::string QMD5_CString(const char *str, size_t n, bool enhanced = false);
std::string QMD5_String(const std::string& str, bool enhanced = false);