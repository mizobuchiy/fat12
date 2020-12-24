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

#include "./read_fat12.h"

int main(int argc, char *argv[]) {
  FILE *fp = NULL;
  const char *read_fname = argc > 1 ? argv[1] : "os20flp.img";

  // open file
  if ((fp = fopen(read_fname, "rb")) == NULL) {
    fprintf(stderr, "can't open %s.\n", read_fname);
    return 1;
  }

  // read file data
  FAT12 fat12;
  if (fread(&fat12, sizeof(unsigned char), sizeof(fat12), fp) < sizeof(fat12)) {
    fprintf(stderr, "can't read %s\n", read_fname);
    return 1;
  }
  printf("done read file %s, len=%ld\n\n", read_fname, sizeof(fat12));
  fclose(fp);

  // read target file by fat12
  const char *target_fname = argc > 2 ? argv[2] : "HELLO.TXT";
  size_t idx;
  if ((idx = find_target_file(fat12.dir_entry, target_fname)) >=
      sizeof(fat12.dir_entry) / sizeof(fat12.dir_entry[0])) {
    fprintf(stderr, "can't find %s\n", target_fname);
    return 1;
  }

  // print file info
  int fat;
  int len;
  print_detail(fat12.dir_entry[idx], &fat, &len);
  print_data(fat, len, fat12);

  return 0;
}
