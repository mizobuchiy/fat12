/**
 * @file fat12.c
 * @brief fat12
 * @copyright Copyright (c) 2020
 *
 */

#include <stdint.h>
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
  if (fread(&fat12, sizeof(uint8_t), sizeof(fat12), fp) < sizeof(fat12)) {
    fprintf(stderr, "can't read %s\n", read_fname);
    return 1;
  }
  printf("done read file %s, len=%ld\n\n", read_fname, sizeof(fat12));
  fclose(fp);

  char *target_fname = argc > 2 ? argv[2] : "HELLO.TXT";
  // read target_fname infomation by fat12
  read_data(fat12, target_fname);

  return 0;
}
