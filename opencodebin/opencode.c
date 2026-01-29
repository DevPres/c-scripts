#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DOCKER_IMAGE "ghcr.io/anomalyco/opencode"
#define DEFAULT_MOUNT "./"
#define DEFAULT_DEST "/workspace"
#define DEFAULT_CONFIG ".config/opencode/"
#define DEFAULT_CONFIG_DEST "/root/.config/opencode"
#define DEFAULT_LOCAL_PATH ".local/share/opencode"
#define DEFAULT_LOCAL_PATH_DEST "/root/.local/share/opencode"

char *ROOT = NULL;

typedef struct {
  char *mount_path;
  char *dest_path;
  char *conf_path;
  char *conf_dest_path;
  char *local_path;
  char *local_dest_path;
  char *work_path;
  char *env_file;
} Config;

void error_exit(const char *message) {
  fprintf(stderr, "Error: %s\n", message);
  exit(EXIT_FAILURE);
}

void print_usage(void) {
  printf("Usage: opencode [OPTIONS]\n");
  printf("Options:\n");
  printf("  -m PATH    Mount path (default: %s)\n", DEFAULT_MOUNT);
  printf("  -d PATH    Destination path (default: %s)\n", DEFAULT_DEST);
  printf("  -c PATH    Configuration path (default %s/%s)\n", ROOT,
         DEFAULT_CONFIG);
  printf("  -w PATH    Working directory (default: same as -d)\n");
  printf("  -e PATH    Environment file path\n");
  printf("  -h         Show this help message\n");
  printf("\nExample:\n");
  printf("  opencode -m /path/to/folder -d /project -w /project -e ~/.env\n");
}

bool create_path_recursive(const char *path) {
  char tmp[PATH_MAX];
  char *p = NULL;
  size_t len;
  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);

  if (tmp[len - 1] == '/') {
    tmp[len - 1] = '\0';
  }
  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return false;
      }
      *p = '/';
    }
  }
  if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
    return false;
  }
  return true;
}

bool validate_and_create_path(const char *path, const char *path_type) {
  if (access(path, F_OK) == 0) {
    return true;
  }

  printf("%s path '%s' does not exist. Create it? (y/n): ", path_type, path);
  char response;
  if (scanf(" %c", &response) != 1) {
    error_exit("Failed to read user input");
  }

  if (response == 'y' || response == 'Y') {
    if (!create_path_recursive(path)) {
      error_exit("Failed to create path");
    }
    printf("Path created successfully.\n");
    return true;
  }

  return false;
}
void parse_arguments(int argc, char *argv[], Config *config) {
  // Set defaults

  config->mount_path = strdup(DEFAULT_MOUNT);
  if (config->mount_path == NULL) {
    error_exit("Memory allocation failed");
  }
  config->dest_path = strdup(DEFAULT_DEST);
  if (config->dest_path == NULL) {
    error_exit("Memory allocation failed");
  }

  int conf_path_len = strlen(ROOT) + 1 + strlen(DEFAULT_CONFIG) + 1;
  config->conf_path = malloc(conf_path_len);
  if (config->conf_path == NULL) {
    error_exit("Memory allocation failed");
  }
  snprintf(config->conf_path, conf_path_len, "%s/%s", ROOT, DEFAULT_CONFIG);
  int local_path_len = strlen(ROOT) + 1 + strlen(DEFAULT_LOCAL_PATH) + 1;
  config->local_path = malloc(local_path_len);
  if (config->local_path == NULL) {
    error_exit("Memory allocation failed");
  }

  snprintf(config->local_path, local_path_len, "%s/%s", ROOT,
           DEFAULT_LOCAL_PATH);
  config->conf_dest_path = strdup(DEFAULT_CONFIG_DEST);
  if (config->conf_dest_path == NULL) {
    error_exit("Memory allocation failed");
  }
  config->local_dest_path = strdup(DEFAULT_LOCAL_PATH_DEST);
  if (config->local_dest_path == NULL) {
    error_exit("Memory allocation failed");
  }
  config->work_path = NULL;
  config->env_file = NULL;

  int opt;
  while ((opt = getopt(argc, argv, "m:d:c:w:e:h")) != -1) {
    switch (opt) {
    case 'm':
      free(config->mount_path);
      config->mount_path = strdup(optarg);
      if (config->mount_path == NULL) {
        error_exit("Memory allocation failed");
      }
      break;
    case 'd':
      free(config->dest_path);
      config->dest_path = strdup(optarg);
      if (config->dest_path == NULL) {
        error_exit("Memory allocation failed");
      }
      break;
    case 'c':
      free(config->conf_path);
      config->conf_path = strdup(optarg);
      if (config->conf_path == NULL) {
        error_exit("Memory allocation failed");
      }
      break;
    case 'w':
      config->work_path = strdup(optarg);
      if (config->work_path == NULL) {
        error_exit("Memory allocation failed");
      }
      break;
    case 'e':
      config->env_file = strdup(optarg);
      if (config->env_file == NULL) {
        error_exit("Memory allocation failed");
      }
      break;
    case 'h':
      print_usage();
      exit(EXIT_SUCCESS);
    default:
      print_usage();
      error_exit("Invalid option");
    }
  }

  // Set working directory default if not specified
  if (config->work_path == NULL) {
    config->work_path = strdup(config->dest_path);
  }
}
char *build_docker_command(const Config *config) {
  // Calculate required buffer size
  size_t size = strlen("docker --rm run -it -v \"") +
                strlen(config->mount_path) + 1 + // ":"
                strlen(config->dest_path) + 1 +  // "\""
                strlen(" -v \"") + strlen(config->local_path) + 1 +
                strlen(config->local_dest_path) + 1 + strlen(" -v \"") +
                strlen(config->conf_path) + 1 + strlen(config->conf_dest_path) +
                1 + strlen(" -w \"") + strlen(config->work_path) + 1 + // "\" "
                strlen(" --name opencode ") +
                strlen(" --add-host=host.docker.internal:host-gateway ") +
                strlen(DOCKER_IMAGE) + 1; // "\0"
  size_t envsize = 0;
  if (config->env_file != NULL) {
    envsize = strlen(" --env-file \" ") + strlen(config->env_file) + 1;
    size += envsize;
  }

  char *command = malloc(size);
  if (command == NULL) {
    error_exit("Memory allocation failed");
  }

  // Build command string
  sprintf(
      command,
      "docker run --rm -it -v \"%s:%s\" -v \"%s:%s\" -v \"%s:%s\"  -w \"%s\" "
      "--add-host=host.docker.internal:host-gateway --name opencode ",
      config->mount_path, config->dest_path, config->local_path,
      config->local_dest_path, config->conf_path, config->conf_dest_path,
      config->work_path);

  if (config->env_file != NULL && envsize > 0) {
    char *tmp = malloc(envsize);
    if (tmp == NULL) {
      error_exit("Memory allocation failed");
    }
    sprintf(tmp, "--env-file \"%s\" ", config->env_file);
    strcat(command, tmp);
    free(tmp);
  }

  strcat(command, DOCKER_IMAGE);
  return command;
}

void execute_command(const char *command) {
  printf("Executing: %s\n", command);

  int result = system(command);
  if (result != 0) {
    error_exit("Docker command failed");
  }
}
void cleanup_config(Config *config) {
  free(config->mount_path);
  free(config->dest_path);
  free(config->conf_path);
  free(config->conf_dest_path);
  free(config->local_path);
  free(config->local_dest_path);
  free(config->work_path);
  free(config->env_file);
}
int main(int argc, char *argv[]) {
  Config config;

  ROOT = getenv("HOME");
  if (ROOT == NULL) {
    error_exit("HOME variables undefined");
  }

  // Parse command-line arguments
  parse_arguments(argc, argv, &config);

  // Validate mount path
  if (!validate_and_create_path(config.mount_path, "Mount")) {
    error_exit("Mount path validation failed");
  }

  // validate conf path
  if (!validate_and_create_path(config.conf_path, "Configuration")) {
    error_exit("Configuration path validation failed");
  }

  if (!validate_and_create_path(config.local_path, "Local")) {
    error_exit("Configuration path validation failed");
  }

  // Validate env file if provided
  if (config.env_file != NULL) {
    if (access(config.env_file, F_OK) != 0) {
      error_exit("Env file not found");
    }
  }

  // Build and execute Docker command
  char *command = build_docker_command(&config);
  execute_command(command);
  // Cleanup
  free(command);
  cleanup_config(&config);

  return 0;
}
