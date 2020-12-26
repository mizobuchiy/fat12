#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef size_t uint_t;

enum { SECTOR_SIZE = 0x200 };

enum attr {
  READ_ONLY = 0x01,
  HIDDEN = 0x02,
  SYSTEM = 0x04,
  VOLUME_ID = 0x08,
  DIRECTORY = 0x10,
  ARCHIVE = 0x20,
  LONG_NAME = 0x0f
};

typedef struct dir_entry {
  uint8_t Name[11];
  uint8_t Attr;
  uint8_t NTRes;
  uint8_t CrtTimeTenth;
  uint8_t CrtTime[2];
  uint8_t CrtDate[2];
  uint8_t LstAccDate[2];
  uint8_t FstClusHI[2];
  uint8_t WrtTime[2];
  uint8_t WrtDate[2];
  uint8_t FstClusLO[2];
  uint8_t FileSize[4];
} DirEntry;

typedef struct fat12 {
  uint8_t ReservedRegion[SECTOR_SIZE];
  uint8_t FATRegion[SECTOR_SIZE * 9];
  uint8_t CopyFATRegion[SECTOR_SIZE * 9];
  DirEntry dir_entry[224];
  uint8_t Data[SECTOR_SIZE * 2847];
} FAT12;

typedef struct long_dir_entry {
  uint8_t Ord;
  uint8_t Name1[10];
  uint8_t Attr;
  uint8_t Type;
  uint8_t Chksum;
  uint8_t Name2[12];
  uint8_t FstClusLo[2];
  uint8_t Name3[4];
} LongDirEntry;

void print_time(uint_t t) {
  printf("%ld:%ld:%ld, ", ((t & 0xf800) >> 11), ((t & 0x7e0) >> 5),
         ((t & 0x1f) << 1));
}

void print_date(uint_t d) {
  printf("%ld/%ld/%ld, ", (((d & 0xfe00) >> 9) + 1980), ((d & 0x1e0) >> 5),
         (d & 0x1f));
}

uint_t to_little_endian(uint8_t *arr, size_t n) {
  uint_t res = 0;
  for (size_t i = 0; i < n; i++) {
    res += arr[i] << (i << 3);
  }
  return res;
}

uint_t read_fat(FAT12 fat12, uint_t fat) {
  size_t j = (fat >> 1) * 3;
  uint_t v = to_little_endian(&fat12.FATRegion[j], 3);
  return (fat % 2 == 0) ? v & 0xfff : (v >> 12) & 0xfff;
}

void print_data(FAT12 fat12, uint_t fat, uint_t len) {
  do {
    size_t base = (fat - 2) * SECTOR_SIZE;

    for (size_t k = 0; (k < SECTOR_SIZE) && (len > 0); len--, k++) {
      printf("%c", fat12.Data[base + k]);
    }
  } while (len > 0 && (fat = read_fat(fat12, fat)) != 0xfff);
  printf("\n");
}

void print_detail(DirEntry dir_entry, uint_t *fat, uint_t *len) {
  print_time(to_little_endian(dir_entry.CrtTime, sizeof(dir_entry.CrtTime)));

  print_date(to_little_endian(dir_entry.CrtDate, sizeof(dir_entry.CrtDate)));

  *fat = to_little_endian(dir_entry.FstClusLO, sizeof(dir_entry.FstClusLO));
  printf("clusLow = %ld, ", *fat);

  *len = to_little_endian(dir_entry.FileSize, sizeof(dir_entry.FileSize));
  printf("size = %ld\n\n", *len);
}

void *my_malloc(size_t size) {
  void *ptr;
  if ((ptr = malloc(size)) == NULL) {
    fprintf(stderr, "failed allocate\n");
    exit(0);
  }
  return ptr;
}

void *my_calloc(size_t nmemb, size_t size) {
  void *ptr;
  if ((ptr = calloc(nmemb, size)) == NULL) {
    fprintf(stderr, "failed allocate\n");
    exit(0);
  }
  return ptr;
}

void set_longdir_name(uint8_t *Name, size_t n, char *dir_name,
                      size_t *dir_name_idx) {
  for (size_t i = 0; i < n; i += 2) {
    if (Name[i] == 0xff) {
      continue;
    }
    dir_name[(*dir_name_idx)++] = Name[i];
  }
}

char *get_dir_name(DirEntry *dir_entry, size_t i, size_t *long_dir_entry_size,
                   bool is_dir) {
  char *dir_name = NULL;
  size_t dir_name_idx = 0;

  // if long name file
  if (dir_entry[i].Attr == LONG_NAME) {
    LongDirEntry long_dir_entry;
    memcpy(&long_dir_entry, &dir_entry[i], sizeof(long_dir_entry));

    // if not end
    if (long_dir_entry.Ord <= 0x40) {
      return dir_name;
    }

    *long_dir_entry_size = long_dir_entry.Ord - 0x40;
    dir_name = my_calloc((13 * (*long_dir_entry_size) / 2), sizeof(char));

    for (size_t j = 0; j < *long_dir_entry_size; j++) {
      memcpy(&long_dir_entry, &dir_entry[(i + *long_dir_entry_size - j - 1)],
             sizeof(long_dir_entry));
      set_longdir_name(long_dir_entry.Name1, sizeof(long_dir_entry.Name1),
                       dir_name, &dir_name_idx);
      set_longdir_name(long_dir_entry.Name2, sizeof(long_dir_entry.Name2),
                       dir_name, &dir_name_idx);
      set_longdir_name(long_dir_entry.Name3, sizeof(long_dir_entry.Name3),
                       dir_name, &dir_name_idx);
    }
    // if shot file name
  } else if (dir_entry[i].Attr == ARCHIVE || dir_entry[i].Attr == DIRECTORY) {
    dir_name = my_calloc(12, sizeof(char));

    for (size_t j = 0; j < sizeof(dir_entry[0].Name); j++) {
      if (dir_entry[i].Name[j] != 0x20) {
        dir_name[dir_name_idx++] = dir_entry[i].Name[j];
      }
      if (j == 7 && !is_dir && dir_name[dir_name_idx - 1] != '.') {
        dir_name[dir_name_idx++] = '.';
      }
    }
  }
  return dir_name;
}

size_t find_entry_idx(DirEntry *dir_entry, size_t entry_size,
                      char *target_dir_name, bool is_dir) {
  for (size_t i = 0; i < entry_size; i++) {
    // if invaliable
    if (dir_entry[i].Name[0] == 0xe5 ||
        (dir_entry[i].Attr != ARCHIVE && dir_entry[i].Attr != DIRECTORY &&
         dir_entry[i].Attr != LONG_NAME)) {
      continue;
    }

    size_t long_dir_entry_size = 0;
    char *dir_name = get_dir_name(dir_entry, i, &long_dir_entry_size, is_dir);

    if (dir_name == NULL) {
      continue;
    }

    // if target_dir_name
    if (strcmp(dir_name, target_dir_name) == 0) {
      printf("name = %s\n", dir_name);
      free(dir_name);
      return i + long_dir_entry_size;
    }
  }

  // if can't find target_dir_name
  fprintf(stderr, "can't find %s\n", target_dir_name);
  exit(0);
}

size_t subdir_wc(FAT12 fat12, uint_t fat) {
  size_t idx = 0;
  do {
    for (size_t k = 0; (k < SECTOR_SIZE); k++) {
      idx++;
    }
  } while ((fat = read_fat(fat12, fat)) != 0xfff);
  return idx;
}

void set_dir_entry(FAT12 fat12, uint_t fat, uint_t len,
                   DirEntry subdir_entry[]) {
  uint8_t *tmp = my_malloc(sizeof(uint8_t) * len);
  size_t idx = 0;
  do {
    size_t base = (fat - 2) * SECTOR_SIZE;

    for (size_t k = 0; (k < SECTOR_SIZE) && (len > 0); len--, k++) {
      tmp[idx++] = fat12.Data[base + k];
    }
  } while (len > 0 && (fat = read_fat(fat12, fat)) != 0xfff);

  memcpy(subdir_entry, tmp, idx);
  free(tmp);
}

void read_data(FAT12 fat12, char *target_fname) {
  DirEntry *dir_entry = my_malloc(sizeof(fat12.dir_entry));

  // initialize dir_entries
  memcpy(dir_entry, fat12.dir_entry, sizeof(fat12.dir_entry));
  size_t entry_size = sizeof(fat12.dir_entry) / sizeof(fat12.dir_entry[0]);

  // for strtok_r
  const char *delim = "/";
  char *save_ptr = NULL;
  char *dir_name;

  // separate by directory
  for (dir_name = strtok_r(target_fname, delim, &save_ptr); dir_name != NULL;
       dir_name = strtok_r(NULL, delim, &save_ptr)) {
    bool is_dir = strcmp(save_ptr, "\0") != 0;

    // print file info
    uint_t fat;
    uint_t len;
    print_detail(
        dir_entry[find_entry_idx(dir_entry, entry_size, dir_name, is_dir)],
        &fat, &len);

    // if file
    if (!is_dir) {
      print_data(fat12, fat, len);
      return;
    }

    // if directory is "../"
    if (fat == 0) {
      continue;
    }

    // if directory
    size_t entry_wc = subdir_wc(fat12, fat);
    free(dir_entry);
    dir_entry = my_malloc(entry_wc);

    set_dir_entry(fat12, fat, entry_wc, dir_entry);
    entry_size = entry_wc / sizeof(fat12.dir_entry[0]);
  }
  free(dir_entry);
  return;
}
