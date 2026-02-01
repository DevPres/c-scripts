#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *pid;
  char *cmdline;
  char *name;
} Proc;

typedef struct {
  Proc *processes;
  size_t size;
  size_t length;
} ProcArray;

void insert_process(char *name, char *pid, char *cmdline, ProcArray *a) {
  if (a->length == a->size) {
    a->size *= 2;
    Proc *new_processes = realloc(a->processes, a->size * sizeof(Proc));
    if (new_processes == NULL) {
      printf("Memory reallocation failed\n");
      exit(EXIT_FAILURE);
    }
    a->processes = new_processes;
  }
  a->processes[a->length].pid = strdup(pid);
  a->processes[a->length].name = name ? strdup(name) : strdup("NULL");
  a->processes[a->length].cmdline = cmdline ? strdup(cmdline) : strdup("NULL");
  a->length++;
}

void init_process_array(ProcArray *a, size_t init_size) {
  a->processes = malloc(init_size * sizeof(Proc));
  a->size = init_size;
  a->length = 0;
}

void free_process_array(ProcArray *a) {
  for (size_t i = 0; i < a->length; i++) {
    free(a->processes[i].pid);
    free(a->processes[i].cmdline);
    free(a->processes[i].name);
  }
  free(a->processes);
  a->processes = NULL;
  a->size = a->length = 0;
}

char *get_cmdline(char *pid, char *f_name) {
  char file_path[256];
  sprintf(file_path, "/proc/%s/%s", pid, f_name);
  FILE *file = fopen(file_path, "r");
  if (file == NULL) {
    printf("Unable to open file %s: %s\n", file_path, strerror(errno));
    return NULL;
  }
  char *cmdline = malloc(4096);
  if (cmdline == NULL) {
    fclose(file);
    return NULL;
  }
  size_t bytes_read = fread(cmdline, sizeof(char), 4096, file);
  if (bytes_read == 0) {
    free(cmdline);
    return NULL;
  }
  fclose(file);
  for (size_t i = 0; i < bytes_read; i++) {
    if (cmdline[i] == '\0')
      cmdline[i] = ' ';
  }
  cmdline[bytes_read] = '\0';

  return cmdline;
}

char *get_name(FILE *file) {
  fseek(file, 0, SEEK_SET);
  char line[256];
  while (fgets(line, 256, file)) {
    char key[100];
    char value[100];
    if (sscanf(line, "%[^:]: %99s", key, value) >= 1) {
      if (strcmp(key, "Name") == 0) {
        return strdup(value);
      };
    }
  }
  return NULL;
}

int get_is_children_process(FILE *file) {
  fseek(file, 0, SEEK_SET);
  char line[256];
  int is_children;
  while (fgets(line, 256, file)) {
    char key[100];
    char value[100];
    if (sscanf(line, "%[^:]: %99s", key, value) >= 1) {
      if (strcmp(key, "PPid") == 0) {
        if (strcmp(value, "1") == 0) {
          is_children = 0;
        } else {
          is_children = 1;
        }
        break;
      }
    }
  }
  return is_children;
}

int main(int argc, char *argv[]) {
  DIR *dir;
  ProcArray processes;

  dir = opendir("/proc");
  if (dir == NULL) {
    printf("Fail to opendir %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  struct dirent *entry;

  init_process_array(&processes, 10);

  printf("Processes: \n");

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

    // Skip non-numeric entries (only process PIDs)
    int is_numeric = 1;
    for (int i = 0; pid[i] != '\0'; i++) {
      if (pid[i] < '0' || pid[i] > '9') {
        is_numeric = 0;
        break;
      }
    }
    if (!is_numeric) {
      free(pid);
      continue;
    }

    char proc_path[256];
    sprintf(proc_path, "/proc/%s", pid);
    DIR *proc = opendir(proc_path);
    if (proc == NULL) {
      if (errno == ENOTDIR) {
        free(pid);
        continue; // Skip non-directories
      }
      printf("Failed to open %s: %s\n", proc_path, strerror(errno));
      return EXIT_FAILURE;
    }
    struct dirent *sub_entry;
    while ((sub_entry = readdir(proc))) {
      char *f_name = strdup(sub_entry->d_name);
      if (f_name == NULL) {
        printf("Memory allocation failed");
        free(pid);
        closedir(dir);
        closedir(proc);
        return EXIT_FAILURE;
      }

      if (strcmp(f_name, "status") == 0) {
        char *file_path = malloc(256);
        if (file_path == NULL) {
          printf("Memory allocation failed\n");
          free(pid);
          closedir(dir);
          closedir(proc);

          exit(EXIT_FAILURE);
        }

        sprintf(file_path, "/proc/%s/%s", pid, f_name);
        FILE *file = fopen(file_path, "r");
        if (file == NULL) {
          printf("Unable to open file %s: %s\n", file_path, strerror(errno));
          free(file_path);
          free(pid);
          closedir(dir);
          closedir(proc);
          return 0;
        }
        int is_children_process = get_is_children_process(file);
        if (is_children_process == 0) {
          char *name = get_name(file);
          char *cmdline = get_cmdline(pid, "cmdline");

          insert_process(name, pid, cmdline, &processes);
          free(name);
          free(cmdline);
        }
        free(f_name);
        free(file_path);
        fclose(file);
        continue;
      }
      free(f_name);
    }
    closedir(proc);
    free(pid);
  }

  closedir(dir);
  for (int i = 0; i < processes.length; i++) {
    char *pid = processes.processes[i].pid;
    char *name = processes.processes[i].name;
    char *cmdline = processes.processes[i].cmdline;
    printf("----\n");
    printf("%s -> %s\n", pid, name);
    printf("%s\n", cmdline);
    printf("----\n");
  }
  free_process_array(&processes);
  return EXIT_SUCCESS;
}
