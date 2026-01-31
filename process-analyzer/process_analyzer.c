#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_cmdline(char *pid, char *f_name) {

  char *cmdline = malloc(4096);
  char file_path[256];
  sprintf(file_path, "/proc/%s/%s", pid, f_name);
  FILE *file = fopen(file_path, "r");
  if (file == NULL) {
    printf("Unable to open file %s: %s\n", file_path, strerror(errno));
    fclose(file);
    free(cmdline);
    return NULL;
  }
  size_t bytes_read = fread(cmdline, sizeof(char), 4096, file);
  fclose(file);
  for (size_t i = 0; i < bytes_read; i++) {
    if (cmdline[i] == '\0')
      cmdline[i] = ' ';
  }
  cmdline[bytes_read] = '\0';

  return cmdline;
}

int main(int argc, char *argv[]) {
  DIR *dir;
  struct dirent *entry;
  struct dirent *sub_entry;

  dir = opendir("/proc");
  if (dir == NULL) {
    printf("Fail to opendir %s\n", strerror(errno));
  }

  printf("Content of directory: \n");

  while ((entry = readdir(dir))) {
    char *pid = strdup(entry->d_name);
    if (pid == NULL) {
      printf("Memory allocation failed\n");
      return EXIT_FAILURE;
    }

    if (strcmp(pid, ".") == 0 || strcmp(pid, "..") == 0) {
      free(pid);
      continue;
    }

    char proc_path[256];
    sprintf(proc_path, "/proc/%s", pid);
    DIR *proc = opendir(proc_path);
    if (proc == NULL) {
      if (errno == ENOTDIR) {
        continue; // Skip non-directories
      }
      printf("Failed to open %s: %s\n", proc_path, strerror(errno));
      return EXIT_FAILURE;
    }
    while ((sub_entry = readdir(proc))) {
      char *f_name = strdup(sub_entry->d_name);
      if (f_name == NULL) {
        printf("Memory allocation failed");
        return EXIT_FAILURE;
      }
      if (strcmp(f_name, "cmdline") == 0) {
        char *cmdline = get_cmdline(pid, f_name);
        printf("%s -> %s\n", pid, cmdline ? cmdline : "Can't retrieve cmdline");
        free(cmdline);
      }
      free(f_name);
    }
    closedir(proc);
    free(pid);
  }

  closedir(dir);

  return EXIT_SUCCESS;
}
