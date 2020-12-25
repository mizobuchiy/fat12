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

typedef struct long_dir_entry {
  unsigned char Ord;
  unsigned char Name1[10];
  unsigned char Attr;
  unsigned char Type;
  unsigned char Chksum;
  unsigned char Name2[12];
  unsigned char FstClusLo[2];
  unsigned char Name3[4];

} LongDirEntry;

void print_time(unsigned int t) {
  printf("%d:%d:%d, ", ((t & 0xf800) >> 11), ((t & 0x7e0) >> 5),
         ((t & 0x1f) << 1));
}

void print_date(unsigned int d) {
  printf("%d/%d/%d, ", (((d & 0xfe00) >> 9) + 1980), ((d & 0x180) >> 5),
         (d & 0x1f));
}

unsigned int byte_swap(unsigned char *arr, size_t n) {
  int res = 0;
  for (size_t i = 0; i < n; i++) {
    res += arr[i] << (i << 3);
  }
  return res;
}

unsigned int read_fat(size_t fat, FAT12 fat12) {
  size_t j = (fat >> 1) * 3;
  size_t v = byte_swap(&fat12.FATRegion[j], 3);
  return (fat % 2 == 0) ? v & 0xfff : (v >> 12) & 0xfff;
}

void print_data(unsigned int fat, unsigned int len, FAT12 fat12) {
  do {
    size_t base = (fat - 2) * SECTOR_SIZE;

    for (size_t k = 0; (k < SECTOR_SIZE) && (len > 0); len--, k++) {
      printf("%c", fat12.Data[base + k]);
    }
  } while (len > 0 && (fat = read_fat(fat, fat12)) != 0xfff);
  printf("\n");
}

void print_detail(DirEntry dir_entry, unsigned int *fat, unsigned int *len) {
  print_time(byte_swap(dir_entry.CrtTime, sizeof(dir_entry.CrtTime)));

  print_date(byte_swap(dir_entry.CrtDate, sizeof(dir_entry.CrtDate)));

  *fat = byte_swap(dir_entry.FstClusLO, sizeof(dir_entry.FstClusLO));
  printf("clusLow = %d, ", *fat);

  *len = byte_swap(dir_entry.FileSize, sizeof(dir_entry.FileSize));
  printf("size = %d\n\n", *len);
}

size_t find_target_file(DirEntry *dir_entry, const char *target_fname,
                        int is_directory) {
  for (size_t i = 0; i < 224; i++) {
    if (dir_entry[i].Name[0] == 0xe5 ||
        (dir_entry[i].Attr != 0x20 && dir_entry[i].Attr != 0x10)) {
      if (dir_entry[i].Attr == 0x0f) {
        LongDirEntry long_dir_entry;
        memcpy(&long_dir_entry, &dir_entry[i], sizeof(long_dir_entry));

        if (long_dir_entry.Ord <= 0x40) {
          continue;
        }

        size_t long_size = long_dir_entry.Ord - 0x40;
        char dir_name[13 * long_size];
        size_t idx;
        size_t name_idx = 0;

        for (size_t j = 0; j < long_size; j++) {
          idx = i + long_size - j - 1;
          memcpy(&long_dir_entry, &dir_entry[idx], sizeof(long_dir_entry));
          // printf("attr:%2x\n", long_dir_entry.Ord);
          for (size_t k = 0; k < sizeof(long_dir_entry.Name1); k += 2) {
            if (long_dir_entry.Name1[k] == 0xff) {
              continue;
            }
            // printf("%c", long_dir_entry.Name1[k]);
            dir_name[name_idx++] = long_dir_entry.Name1[k];
          }
          for (size_t k = 0; k < sizeof(long_dir_entry.Name2); k += 2) {
            if (long_dir_entry.Name2[k] == 0xff) {
              continue;
            }
            // printf("%c", long_dir_entry.Name2[k]);
            dir_name[name_idx++] = long_dir_entry.Name2[k];
          }
          for (size_t k = 0; k < sizeof(long_dir_entry.Name3); k += 2) {
            if (long_dir_entry.Name3[k] == 0xff) {
              continue;
            }
            // printf("%c", long_dir_entry.Name3[k]);
            dir_name[name_idx++] = long_dir_entry.Name3[k];
          }
        }
        if (strcmp(dir_name, target_fname) == 0) {
          printf("name = %s\n", dir_name);
          return i + long_size;
        }
      }
      continue;
    }
    char dir_name[13] = "";
    size_t idx = 0;
    for (size_t j = 0; j < sizeof(dir_entry[0].Name); j++) {
      if (dir_entry[i].Name[j] != 0x20) {
        dir_name[idx++] = dir_entry[i].Name[j];
      }
      if (j == 7 && !is_directory) {
        dir_name[idx++] = '.';
      }
    }
    if ((dir_entry[i].Attr == 0x20 || dir_entry[i].Attr == 0x10) &&
        strcmp(dir_name, target_fname) == 0) {
      printf("name = %s\n", dir_name);
      return i;
    }
  }
  return -1;
}

size_t get_subdir_size(unsigned int fat, FAT12 fat12) {
  size_t idx = 0;
  do {
    for (size_t k = 0; (k < SECTOR_SIZE); k++) {
      idx++;
    }
  } while ((fat = read_fat(fat, fat12)) != 0xfff);
  return idx;
}

void set_dir(unsigned int fat, unsigned int len, FAT12 fat12,
             DirEntry subdir_entry[]) {
  unsigned char *tmp = (unsigned char *)malloc(sizeof(unsigned char) * len);
  size_t idx = 0;
  do {
    int base = (fat - 2) * SECTOR_SIZE;

    for (size_t k = 0; (k < SECTOR_SIZE) && (len > 0); len--, k++) {
      tmp[idx++] = fat12.Data[base + k];
    }
  } while (len > 0 && (fat = read_fat(fat, fat12)) != 0xfff);

  memcpy(subdir_entry, tmp, idx);
  free(tmp);
}

void read_long_fname() { return; }
