#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>

#define SERVER_PORT "6666" 
#define SERVER_ADDR "2804:1b3:a701:5e11:b025:5a85:c0b5:7355" // must hardcode IP address
#define BUF_SIZE 1024 
#define MAX_ARGS 10

// Helper function to split client input 
int split_args(char *input, char **args, int max_args) {
    int argc = 0;
    char *p = input;

    while (*p && argc < max_args) {
        while (*p == ' ') p++;  
        if (*p == 0) break;
        if (*p == '"') {
            p++; 
            args[argc] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') {
                *p = 0;  
                p++;
            }
            argc++;
        } else {
            args[argc] = p;
            while (*p && *p != ' ') p++;
            if (*p == ' ') {
                *p = 0;
                p++;
            }
            argc++;
        }
    } 

    return argc;
}

// helper function to read the data from sockfd into resp until receiving END_OF_RESPONSE
int read_response(int sockfd, char *resp, int resp_size) {
    static char leftover[BUF_SIZE] = "";
    int total = 0;
    
    if (strlen(leftover) > 0) {
        strncpy(resp, leftover, resp_size - 1);
        resp[resp_size - 1] = '\0';
        total = strlen(resp);
        leftover[0] = '\0';
    } else {
        resp[0] = '\0';
        total = 0;
    }
    
    while (1) {
        char *marker = strstr(resp, "END_OF_RESPONSE\n");
        if (marker) {
            *marker = '\0';
            marker += strlen("END_OF_RESPONSE\n");
            if (*marker != '\0') {
                strncpy(leftover, marker, BUF_SIZE - 1);
                leftover[BUF_SIZE - 1] = '\0';
            }
            break;
        }
        // read data until receive marker
        int bytes = recv(sockfd, resp + total, resp_size - total - 1, 0);
        if (bytes <= 0) {
            break;
        }
        total += bytes;
        resp[total] = '\0';
    }
    return total;
}

void printHelpInfo(){
    printf("Available commands:\n");
    printf("  addMovie title genre director debutDate -> Adds movie to database\nGenres must be in format 'genre1,genre2,...', and data must be in format YYYY-MM-DD\n");
    printf("  addGenre id genre -> adds genre to movie with specified id if it exists\n");
    printf("  remove id -> remove movie with specified id\n");
    printf("  list (argument) (optional)\n");
    printf("    arguments:\n");
    printf("      -all -> List entire database\n");
    printf("      -movies -> List all movies alongside their IDs\n");
    printf("      -i id -> List information about a specific movie, must give a INT as ID\n");
    printf("      -g genre -> List all movies of specified genre\n");
    printf("  -h, help -> Show help message\n");
    printf("  exit -> Exit program\n");
}

int main() {
    // Networking variables
    struct addrinfo hints, *res;
    int sockfd;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     
    hints.ai_socktype = SOCK_STREAM; 

    status = getaddrinfo(SERVER_ADDR, SERVER_PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        freeaddrinfo(res);
        return 1;
    }

    status = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (status == -1) {
        fprintf(stderr, "connect error: %s\n", strerror(errno));
        close(sockfd);
        freeaddrinfo(res);
        return 1;
    }

    // Connected, now the user can request infos
    printf("Welcome to MovieDB, you are now connected to the land of movies information!\nFor exiting type 'exit' \nType -h if you need help :)\n\n");

    char IObuffer[BUF_SIZE];
    static char request_buf[BUF_SIZE]; 
    char *request;
    char *args[MAX_ARGS];

    while (1){
        memset(IObuffer, 0, BUF_SIZE);
        printf(">> ");
        if (fgets(IObuffer, BUF_SIZE, stdin) == NULL) {
            break;
        }
        IObuffer[strcspn(IObuffer, "\n")] = 0;  

        int argc = split_args(IObuffer, args, MAX_ARGS);
        if (argc == 0) continue;

        char *command = args[0];
        request = request_buf; 

        if (strcmp(command, "exit") == 0) {
            break;
        } 
        else if (strcmp(command, "addMovie") == 0) {
            if (argc != 5) {
                printf("Usage: addMovie title genres director debutDate\n");
                continue;
            }
            snprintf(request_buf, BUF_SIZE, "ADDMOVIE \"%s\" \"%s\" \"%s\" \"%s\"", args[1], args[2], args[3], args[4]);
            // printf("Request prepared: %s\n", request);
        }
        else if (strcmp(command, "addGenre") == 0) {
            if (argc != 3) {
                printf("Usage: addGenre id genre\n");
                continue;
            }
            snprintf(request_buf, BUF_SIZE, "ADDGENRE %s %s", args[1], args[2]);
            // printf("Request prepared: %s\n", request);
        }
        else if (strcmp(command, "remove") == 0) {
            if (argc != 2) {
                printf("Usage: remove id\n");
                continue;
            }
            snprintf(request_buf, BUF_SIZE, "REMOVE %s", args[1]);
            // printf("Request prepared: %s\n", request);
        }
        else if (strcmp(command, "list") == 0) {
            if (argc == 1) {
                strcpy(request_buf, "LIST ALL");
                // printf("Request prepared: %s\n", request);
            }
            else if (argc == 2) {
                if (strcmp(args[1], "-all") == 0) {
                    strcpy(request_buf, "LIST ALL");
                    // printf("Request prepared: %s\n", request);
                }
                else if (strcmp(args[1], "-movies") == 0) {
                    strcpy(request_buf, "LIST MOVIES");
                    // printf("Request prepared: %s\n", request);
                }
                else {
                    printf("Invalid list option.\n");
                    continue;
                }
            }
            else if (argc == 3) {
                if (strcmp(args[1], "-i") == 0) {
                    snprintf(request_buf, BUF_SIZE, "LIST ID %s", args[2]);
                    // printf("Request prepared: %s\n", request);
                }
                else if (strcmp(args[1], "-g") == 0) {
                    snprintf(request_buf, BUF_SIZE, "LIST GENRE %s", args[2]);
                    // printf("Request prepared: %s\n", request);
                }
                else {
                    printf("Invalid list option.\n");
                    continue;
                }
            }
            else {
                printf("Invalid list command\n");
                continue;
            }
        }
        else if (strcmp(command, "-h") == 0 || strcmp(command, "help") == 0) {
            printHelpInfo();
            continue;
        }
        else {
            printf("Invalid command\n");
            continue;
        }
    
        // Send request 
        ssize_t bytes_sent = send(sockfd, request, strlen(request), 0);
        if (bytes_sent == -1) {
            fprintf(stderr, "send error: %s\n", strerror(errno));
            break;
        }
        // printf("request sent!\n");
        
        // Read the response until receiveng the termination marker
        char response[BUF_SIZE];
        memset(response, 0, BUF_SIZE);
        read_response(sockfd, response, BUF_SIZE);
        printf("%s\n", response); 
    }

    // Cleanup
    close(sockfd);
    freeaddrinfo(res);
    return 0;
}