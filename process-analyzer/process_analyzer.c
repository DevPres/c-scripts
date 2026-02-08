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
      fprintf(stderr, "Memory reallocation failed\n");
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
  snprintf(file_path, sizeof(file_path), "/proc/%s/%s", pid, f_name);
  FILE *file = fopen(file_path, "r");
  if (file == NULL) {
    fprintf(stderr, "Unable to open file %s: %s\n", file_path, strerror(errno));
    return NULL;
  }
  char *cmdline = malloc(4097);
  if (cmdline == NULL) {
    fclose(file);
    return NULL;
  }
  size_t bytes_read = fread(cmdline, sizeof(char), 4096, file);
  if (bytes_read == 0) {
    free(cmdline);
    fclose(file);
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
    char key[20];
    char value[20];
    if (sscanf(line, "%19[^:]: %19s", key, value) >= 1) {
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
  int is_child = 0;
  while (fgets(line, 256, file)) {
    char key[20];
    char value[20];
    if (sscanf(line, "%19[^:]: %19s", key, value) >= 1) {
      if (strcmp(key, "PPid") == 0) {
        if (strcmp(value, "1") == 0) {
          is_child = 0;
        } else {
          is_child = 1;
        }
        break;
      }
    }
  }
  return is_child;
}

int main(int argc, char *argv[]) {
  DIR *dir;
  ProcArray processes;

  char *pid_info = getenv("ROFI_INFO");
  if (pid_info != NULL && argc > 1 && strcmp(argv[1], "YES") == 0) {
    printf("killed: %s\n", pid_info);
    return 0;
  }
  if (pid_info != NULL) {
    printf("YES");
    putchar('\0');
    printf("info\x1f%s\x1f\n", pid_info);

    printf("NO\n");
    return 0;
  }
  // If they clicked a PID for the first time, show confirmation menu
  // We pass the PID forward again using 'info' so we don't lose it

  dir = opendir("/proc");
  if (dir == NULL) {
    fprintf(stderr, "Fail to opendir %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  struct dirent *entry;

  init_process_array(&processes, 10);

  printf("Processes: \n");

  while ((entry = readdir(dir))) {
    char *pid = entry->d_name;

    // if (strcmp(pid, ".") == 0 || strcmp(pid, "..") == 0) {
    //   continue;
    // }

    // Skip non-numeric entries (only process PIDs)
    int is_numeric = 1;
    for (int i = 0; pid[i] != '\0'; i++) {
      if (pid[i] < '0' || pid[i] > '9') {
        is_numeric = 0;
        break;
      }
    }
    if (!is_numeric) {
      continue;
    }

    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/%s", pid);
    DIR *proc = opendir(proc_path);
    if (proc == NULL) {
      if (errno == ENOTDIR) {
        continue; // Skip non-directories
      }
      fprintf(stderr, "Failed to open %s: %s\n", proc_path, strerror(errno));
      return EXIT_FAILURE;
    }
    struct dirent *sub_entry;
    while ((sub_entry = readdir(proc))) {
      char *f_name = sub_entry->d_name;
      if (f_name == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        closedir(dir);
        closedir(proc);
        return EXIT_FAILURE;
      }

      if (strcmp(f_name, "status") == 0) {
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "/proc/%s/%s", pid, f_name);

        FILE *file = fopen(file_path, "r");
        if (file == NULL) {
          fprintf(stderr, "Unable to open file %s: %s\n", file_path,
                  strerror(errno));
          closedir(dir);
          closedir(proc);
          return EXIT_FAILURE;
        }
        int is_children_process = get_is_children_process(file);
        if (is_children_process == 0) {
          char *name = get_name(file);
          char *cmdline = get_cmdline(pid, "cmdline");

          insert_process(name, pid, cmdline, &processes);
          free(name);
          free(cmdline);
        }
        fclose(file);
        continue;
      }
    }
    closedir(proc);
  }

  closedir(dir);
  for (size_t i = 0; i < processes.length; i++) {
    char *pid = processes.processes[i].pid;
    char *name = processes.processes[i].name;
    char *cmdline = processes.processes[i].cmdline;
    // printf("----\n");
    printf("%s -> %s", pid, name);
    // printf("%s\n", cmdline);
    // printf("----\n");
    putchar('\0');
    printf("info\x1f%s\x1f\n", pid);
  }
  free_process_array(&processes);
  return EXIT_SUCCESS;
}
