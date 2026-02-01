#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *pid;
  char *cmdline;
} Proc;

typedef struct {
  Proc *processes;
  size_t size;
  size_t length;
} ProcArray;

void insert_process(char *pid, char *cmdline, ProcArray *a) {
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
  if (cmdline != NULL) {
    a->processes[a->length].cmdline = strdup(cmdline);
  } else {
    a->processes[a->length].cmdline = NULL;
  }
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
    return "Can retrieve cmdline";
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

// int check_if_descendant(char *pid, char *f_name) {
//   char *file_path = malloc(256);
//   if (file_path == NULL) {
//     printf("Memory allocation failed\n");
//     exit(EXIT_FAILURE);
//   }
//
//   sprintf(file_path, "/proc/%s/%s", pid, f_name);
//   FILE *file = fopen(file_path, "r");
//   if (file == NULL) {
//     printf("Unable to open file %s: %s\n", file_path, strerror(errno));
//     return 0;
//   }
//   char *gid = malloc(256);
//   if (gid == NULL) {
//     printf("Memory allocation failed\n");
//     exit(EXIT_FAILURE);
//   }
//
//   char *line = malloc(256);
//   if (line == NULL) {
//     printf("Memory allocation failed\n");
//     exit(EXIT_FAILURE);
//   }
//
//   char *gid_key = "NStgid";
//
//   while (fgets(line, 256, file)) {
//     char key[100];
//     sscanf(line, "%[^:]: %10s", key, gid);
//     if (strcmp(key, gid_key) == 0) {
//       break;
//     }
//   }
//   int is_not_descendant = strcmp(gid, pid) == 0;
//   fclose(file);
//   free(file_path);
//   free(gid);
//   free(line);
//   if (is_not_descendant) {
//     return 1;
//   }
//
//   return 0;
// }

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
    sprintf(proc_path, "/proc/%s", pid);
    DIR *proc = opendir(proc_path);
    if (proc == NULL) {
      if (errno == ENOTDIR) {
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
        return EXIT_FAILURE;
      }

      if (strcmp(f_name, "status") == 0) {
        char *cmdline = get_cmdline(pid, "cmdline");

        insert_process(pid, cmdline, &processes);
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
    char *cmdline = processes.processes[i].cmdline;
    printf("%s -> %s\n", pid, cmdline);
  }
  free_process_array(&processes);
  return EXIT_SUCCESS;
}
