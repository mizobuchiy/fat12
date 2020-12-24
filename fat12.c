/**
 * @file fat12.c
 * @author Yuya Mizobuchi (18D8101034F)
 * @brief
 * @copyright Copyright (c) 2020
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int to_int(unsigned char *arr, size_t n) {
  int res = 0;
  for (size_t i = 0; i < n; i++) {
    res += arr[i] << (i << 3);
  }
  return res;
}

int read_fat(int i, unsigned char *data) {
  int j = (i >> 1) * 3;
  int v = 0;
  int fat0 = 0x200;
  for (int k = 2; k >= 0; k--) {
    v = (v << 8) + data[fat0 + j + k];
  }
  return (i % 2 == 0) ? v & 0xfff : (v >> 12) & 0xfff;
}

int main(int argc, char const *argv[]) {
  FILE *fp = NULL;
  const char read_fname[] = "os20flp.img";

  if ((fp = fopen(read_fname, "rb")) == NULL) {
    fprintf(stderr, "can't open %s.\n", read_fname);
    return 1;
  }

  FAT12 fat12;
  if (fread(&fat12, sizeof(unsigned char), sizeof(fat12), fp) < sizeof(fat12)) {
    fprintf(stderr, "can't read %s\n", read_fname);
    return 1;
  }
  printf("done read file %s len=%ld\n\n", read_fname, sizeof(fat12));
  fclose(fp);

  // unsigned char data[0x200 * 0xb40];
  // if (fread(data, sizeof(data[0]), sizeof(data), fp) < sizeof(data)) {
  //   fprintf(stderr, "can't read %s\n", read_fname);
  //   return 1;
  // }
  // printf("done read file %s len=%ld\n\n", read_fname, sizeof(data));
  // fclose(fp);

  // DirEntry dir_entry[0xe0];
  // for (size_t i = 0; i < sizeof(dir_entry) / sizeof(dir_entry[0]); i++) {
  //   memcpy(&dir_entry[i], &data[0x2600 + (i * sizeof(dir_entry[0]))],
  //          sizeof(dir_entry[0]));
  // }

  // const char target_fname[] = "HELLO.TXT";
  // for (size_t i = 0; i < sizeof(dir_entry) / sizeof(dir_entry[0]); i++) {
  //   if (dir_entry[i].Name[i] == 0xe5 || dir_entry[i].Attr != 0x20) {
  //     continue;
  //   }
  //   char dir_name[13] = "";
  //   size_t idx = 0;
  //   for (size_t j = 0; j < sizeof(dir_entry[0].Name); j++) {
  //     if (dir_entry[i].Name[j] != 0x20) {
  //       dir_name[idx++] = dir_entry[i].Name[j];
  //     }
  //     if (j == 7) {
  //       dir_name[idx++] = '.';
  //     }
  //   }

  //   if (dir_entry[i].Attr == 0x20 && strcmp(dir_name, target_fname) == 0) {
  //     printf("name = %s\n", dir_name);

  //     const int t = to_int(dir_entry[i].CrtTime,
  //     sizeof(dir_entry[i].CrtTime)); printf("%d:%d:%d ", ((t & 0xf800) >>
  //     11), ((t & 0x7e0) >> 5),
  //            ((t & 0x1f) << 1));

  //     const int d = to_int(dir_entry[i].CrtDate,
  //     sizeof(dir_entry[i].CrtDate)); printf("%d/%d/%d ", (((d & 0xfe00) >> 9)
  //     + 1980), ((d & 0x180) >> 5),
  //            (d & 0x1f));

  //     const int fat =
  //         to_int(dir_entry[i].FstClusLO, sizeof(dir_entry[i].FstClusLO));
  //     printf("clusLow = %d ", fat);

  //     const int len =
  //         to_int(dir_entry[i].FileSize, sizeof(dir_entry[i].FileSize));
  //     printf("size = %d\n", len);

  //     int f = fat;
  //     int l = len;
  //     do {
  //       int base = f * 0x200 + 0x3e00;
  //       for (size_t k = 0; (k < 0x200) && (l > 0); l--, k++) {
  //         printf("%c", data[base + k]);
  //       }
  //     } while (l > 0 && (f = read_fat(f, data)) != 0xfff);
  //   }
  // }

  return 0;
}
