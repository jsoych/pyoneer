#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "server.h"
#include "site.h"
#include "pyoneer.h"

Site* SITE = NULL;
int ID = 0;
int ROLE = -1;
int LEVEL = LOGGER_INFO;

// Parses the flags and sets ID, ROLE and LEVEL
int parse_flags(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    // Check argument count
    if (argc < 2) {
        fprintf(stderr, "main: Error: Expected at least 1 argument\n");
        exit(EXIT_FAILURE);
    }

    if (parse_flags(argc, argv) == -1) {
        fprintf(stderr, "main: Error: Unable to parse flags\n");
        exit(EXIT_FAILURE);
    }
    
    if (ROLE == PYONEER_WORKER) {
        SITE = site_create(
            getenv("PYONEER_PYTHON"),
            getenv("PYONEER_TASK_DIR"),
            getenv("PYONEER_WORKING_DIR")
        );
        if (!SITE) {
            fprintf(stderr, "main: Error: Missing environment variables\n");
            exit(EXIT_FAILURE);
        }
    }

    Logger* logger = logger_create(LEVEL, "API: %s\n");
    Pyoneer* pyoneer = pyoneer_create(ROLE, ID);

    // Create API server
    char* socket_path = getenv("PYONEER_SOCKET_PATH");
    char* token = getenv("PYONEER_API_TOKEN");
    if (!socket_path || !token) {
        fprintf(stderr, "main: Error: Missing environment variables\n");
        logger_destroy(logger);
        pyoneer_destroy(pyoneer);
        exit(EXIT_FAILURE);
    }

    Server* server = server_create(pyoneer, logger, socket_path, token);
    if (!server) {
        fprintf(stderr, "main: Error: Unable to create server\n");
        logger_destroy(logger);
        pyoneer_destroy(pyoneer);
        exit(EXIT_FAILURE);
    }

    // Start server
    if (server_start(server) == -1) {
        fprintf(stderr, "main: Error: Unable to start server\n");
    }

    // Clean up
    server_destroy(server);
    pyoneer_destroy(pyoneer);
    logger_destroy(logger);
    site_destroy(SITE);
    exit(EXIT_SUCCESS);
}

bool check_flag(const char* start, unsigned int len, const char* rest) {
    if (strlen(start) < len) return false;
    return (memcmp(start, rest, len) == 0);
}

char* next(char* str, int pos) {
    return &str[pos];
}

int parse_flags(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        char* flag = argv[i];
        if (!check_flag(flag, 2, "--")) return -1;
        flag = next(flag, 2);
        switch (*flag) {
            case 'd':
                if (!check_flag(flag, 5, "debug")) return -1;
                LEVEL = LOGGER_DEBUG;
                break;
            case 'i':
                if (!check_flag(flag, 3, "id=")) return -1;
                flag = next(flag, 3);
                ID = strtol(flag, NULL, 10);
                break;
            case 'r':
                if (!check_flag(flag, 5, "role=")) return -1;
                flag = next(flag, 5);
                switch (*flag) {
                    case 'm':
                        if (!check_flag(flag, 7, "manager")) return -1;
                        ROLE = PYONEER_MANAGER;
                        break;
                    case 'w':
                        if (!check_flag(flag, 6, "worker")) return -1;
                        ROLE = PYONEER_WORKER;
                        break;
                    default:
                        fprintf(stderr, "main: Unknown role (%s)\n", flag);
                        return -1;
                }
                break;
            default:
                fprintf(stderr, "main: Unknown flag (%s)\n", flag);
                return -1;
        }
    }
    return 0;
}
