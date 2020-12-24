/**
 * @file file_system.h
 * @brief file system by fat12
 * @copyright Copyright (c) 2020
 *
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>

enum { SECTOR_SIZE = 0x200 };

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
  unsigned char ReservedRegion[SECTOR_SIZE];
  unsigned char FATRegion[SECTOR_SIZE * 9];
  unsigned char CopyFATRegion[SECTOR_SIZE * 9];
  DirEntry dir_entry[224];
  unsigned char Data[SECTOR_SIZE * 2847];
} FAT12;

int read_fat(int i, FAT12 fat12) {
  int j = (i >> 1) * 3;
  int v = 0;
  for (int k = 2; k >= 0; k--) {
    v = (v << 8) + fat12.FATRegion[j + k];
  }
  return (i % 2 == 0) ? v & 0xfff : (v >> 12) & 0xfff;
}

void print_time(int t) {
  printf("%d:%d:%d, ", ((t & 0xf800) >> 11), ((t & 0x7e0) >> 5),
         ((t & 0x1f) << 1));
}

void print_date(int d) {
  printf("%d/%d/%d, ", (((d & 0xfe00) >> 9) + 1980), ((d & 0x180) >> 5),
         (d & 0x1f));
}

void print_data(int fat, int len, FAT12 fat12) {
  do {
    int base = (fat - 33) * SECTOR_SIZE + 0x3e00;
    for (size_t k = 0; (k < SECTOR_SIZE) && (len > 0); len--, k++) {
      printf("%c", fat12.Data[base + k]);
    }
  } while (len > 0 && (fat = read_fat(fat, fat12)) != 0xfff);
  printf("\n");
}

int byte_swap(unsigned char *arr, size_t n) {
  int res = 0;
  for (size_t i = 0; i < n; i++) {
    res += arr[i] << (i << 3);
  }
  return res;
}

void print_detail(DirEntry dir_entry, int *fat, int *len) {
  print_time(byte_swap(dir_entry.CrtTime, sizeof(dir_entry.CrtTime)));

  print_date(byte_swap(dir_entry.CrtDate, sizeof(dir_entry.CrtDate)));

  *fat = byte_swap(dir_entry.FstClusLO, sizeof(dir_entry.FstClusLO));
  printf("clusLow = %d, ", *fat);

  *len = byte_swap(dir_entry.FileSize, sizeof(dir_entry.FileSize));
  printf("size = %d\n\n", *len);
}

size_t find_target_file(DirEntry *dir_entry, const char *target_fname) {
  for (size_t i = 0; i < 224; i++) {
    if (dir_entry[i].Name[i] == 0xe5 || dir_entry[i].Attr != 0x20) {
      continue;
    }
    char dir_name[13] = "";
    size_t idx = 0;
    for (size_t j = 0; j < sizeof(dir_entry[0].Name); j++) {
      if (dir_entry[i].Name[j] != 0x20) {
        dir_name[idx++] = dir_entry[i].Name[j];
      }
      if (j == 7) {
        dir_name[idx++] = '.';
      }
    }

    if (dir_entry[i].Attr == 0x20 && strcmp(dir_name, target_fname) == 0) {
      printf("name = %s\n", dir_name);
      return i;
    }
  }
  return -1;
}
