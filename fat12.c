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
  // char *target_fname = argc > 2 ? argv[2] : "HELLO.TXT";
  char *target_fname = argc > 2 ? argv[2] : "EXDIR1/18D1034F.TXT";
  const char *delim = "/";
  char *save_ptr = NULL;
  char *token;

  DirEntry *dir_entries = NULL;
  if ((dir_entries = malloc(sizeof(fat12.dir_entry))) == NULL) {
    fprintf(stderr, "failed allocate\n");
    exit(0);
  }
  size_t entry_size = sizeof(fat12.dir_entry);
  memcpy(dir_entries, fat12.dir_entry, entry_size);

  for (token = strtok_r(target_fname, delim, &save_ptr); token != NULL;
       token = strtok_r(NULL, delim, &save_ptr)) {
    size_t idx;
    if ((idx = find_target_file(dir_entries, token,
                                strcmp(save_ptr, "\0") == 0 ? 0 : 1)) >=
        entry_size) {
      fprintf(stderr, "can't find %s\n", token);
      return 1;
    }

    // print file info
    unsigned int fat;
    unsigned int len;
    print_detail(dir_entries[idx], &fat, &len);

    // if file
    if (strcmp(save_ptr, "\0") == 0) {
      print_data(fat, len, fat12);
      return 0;
      // if directory
    }
    entry_size = get_subdir_size(fat, fat12);
    dir_entries = NULL;
    if ((dir_entries = malloc(entry_size)) == NULL) {
      fprintf(stderr, "failed allocate\n");
      exit(0);
    }

    set_dir(fat, entry_size, fat12, dir_entries);
  }
  free(dir_entries);
  return 0;
}
