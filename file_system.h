/**
 * @file file_system.h
 * @brief file system by fat12
 * @copyright Copyright (c) 2020
 *
 */
#pragma once

typedef struct dir_entry {
  unsigned char Name[11];
  unsigned char Attr;
  unsigned char NTRes;
  unsigned char CrtTimeTenth;
  unsigned char CrtTime[2];
  unsigned char CrtDate[2];
  unsigned char LstAccDate[2];
  unsigned char FstClusHI[2];
  unsigned char WrtTime[2];
  unsigned char WrtDate[2];
  unsigned char FstClusLO[2];
  unsigned char FileSize[4];
} DirEntry;

typedef struct fat12 {
  unsigned char ReservedRegion[0x200];
  unsigned char FATRegion[0x200 * 9];
  unsigned char CopyFATRegion[0x200 * 9];
  DirEntry dir_entry[0xe0];
  unsigned char Data[0x200 * 2847];
} FAT12;
