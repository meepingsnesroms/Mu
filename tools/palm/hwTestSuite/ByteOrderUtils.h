/******************************************************************************
 *
 * Copyright (c) 1995-2002 PalmSource, Inc. All rights reserved.
 *
 * File: utils68K.h
 *
 * Release: eclipse 5 SDK (68K) R4.
 *
 *****************************************************************************/

#ifndef __BYTEORDERUTILS_H__
#define __BYTEORDERUTILS_H__

#define EndianSwap16(n)	(((((unsigned int) n) << 8) & 0xFF00) | ((((unsigned int) n) >> 8) & 0x00FF))


#define EndianSwap32(n)	(((((unsigned long) n) << 24) & 0xFF000000) | ((((unsigned long) n) <<  8) & 0x00FF0000) | ((((unsigned long) n) >>  8) & 0x0000FF00) |	((((unsigned long) n) >> 24) & 0x000000FF))

#define Read68KUnaligned32(addr) (((((unsigned char *)(addr))[0]) << 24) | ((((unsigned char *)(addr))[1]) << 16) | ((((unsigned char *)(addr))[2]) << 8) | ((((unsigned char *)(addr))[3])))

#define Write68KUnaligned32(addr, value) (((unsigned char *)(addr))[0] = (unsigned char)((unsigned long)(value) >> 24), ((unsigned char *)(addr))[1] = (unsigned char)((unsigned long)(value) >> 16), (unsigned char *)(addr))[2] = (unsigned char)((unsigned long)(value) >> 8), ((unsigned char *)(addr))[3] = (unsigned char)((unsigned long)(value)))

#endif
