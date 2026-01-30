#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  DIR *dir;
  struct dirent *entry;

  dir = opendir("/proc");
  if (dir == NULL) {
    printf("Fail to opendir %s\n", strerror(errno));
  }

  printf("Content of directory: \n");

  while ((entry = readdir(dir))) {
    char *dname = strdup(entry->d_name);
    if (dname == NULL) {
      printf("Memory allocation failed\n");
      return EXIT_FAILURE;
    }

    if (strcmp(dname, ".") == 0 || strcmp(dname, "..") == 0) {
      continue;
    }

    char proc_path[256];
    sprintf(proc_path, "/proc/%s", dname);
    DIR *proc = opendir(proc_path);
    if (proc == NULL) {
      if (errno == ENOTDIR) {
        continue; // Skip non-directories
      }
      printf("Failed to open %s: %s\n", proc_path, strerror(errno));
      return EXIT_FAILURE;
    }
    while ((entry = readdir(proc))) {
      char *pdname = strdup(entry->d_name);
      if (pdname == NULL) {
        printf("Memory allocation failed");
        return EXIT_FAILURE;
      }
      if (strcmp(pdname, "cmdline") == 0) {
        char file_path[256];
        sprintf(file_path, "/proc/%s/%s", dname, pdname);
        FILE *file = fopen(file_path, "r");
        if (file == NULL) {
          printf("Unable to open file %s\n: %s\n", file_path, strerror(errno));
          return EXIT_FAILURE;
        }
        int ch;
        FILE *test = fopen("/home/dario/test", "w");
        printf("%s -> ", dname);
        // Loop through the file and print each character until
        // EOF is reached
        while ((ch = fgetc(file)) != EOF) {
          if (ch != '\0') {
            putchar(ch);
          }
        }
        printf("\n");
        fclose(file);
        free(pdname);
        break;
      }
    }
    closedir(proc);
    free(dname);
  }

  closedir(dir);

  return EXIT_SUCCESS;
}
