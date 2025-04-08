#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<netdb.h>
#include<sys/types.h>
#include<sys/socket.h>
#include <ctype.h>

#include<sqlite3.h>
#include<pthread.h>

#define MYPORT "6666"
#define BACKLOG 20
#define BUF_SIZE 1024
#define MAX_ARGS 10

sqlite3 *db;

// Helper function to split the client request
int split_request(char *buffer, char **args, int max_args) {
    int argc = 0;          
    char *p = buffer;     
    int in_quote = 0;   

    while (*p && argc < max_args) {
        while (isspace((unsigned char)*p)) p++;
        if (*p == 0) break; 
        args[argc] = p;
        argc++;
        while (*p) {
            if (*p == '"') {
                in_quote = !in_quote;  
                memmove(p, p + 1, strlen(p + 1) + 1);
                continue;  
            } else if (in_quote) {
                p++;
            } else if (isspace((unsigned char)*p)) {
                *p = 0;  
                p++;
                break;
            } else {
                p++;
            }
        }
    }

    return argc;
}


// Callback for db
static int callback(void *data, int argc, char **argv, char **azColName){
    int client_fd = *(int *)data;
    char buf[BUF_SIZE];
    for(int i=0; i<argc; i++){
       snprintf(buf, sizeof buf, "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
       send(client_fd, buf, strlen(buf), 0);
    }
    send(client_fd, "\n", 1, 0);
    return 0;
}

// Add movie to the database
int addMovie(sqlite3 *db, char *title, char *genres, char *director, char *debutDate, char **response){
    sqlite3_stmt *stmt;
    int rc;
    const char *sql = "INSERT INTO movies (title, genres, director, debutDate) VALUES (?, ?, ?, ?);";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *response = malloc(BUF_SIZE);
        snprintf(*response, BUF_SIZE, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    } 

    // printf("title is %s, genres is %s, director is %s, and debutDate is %s\n", title, genres, director, debutDate);

    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, genres, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, director, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, debutDate, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);  
        *response = malloc(BUF_SIZE);
        snprintf(*response, BUF_SIZE, "Failed to add movie: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    // char *expanded_sql = sqlite3_expanded_sql(stmt);
    // if (expanded_sql) {
    //     printf("Expanded SQL: %s\n", expanded_sql);
    //     sqlite3_free(expanded_sql);  
    // } else {
    //     printf("Failed to expand SQL statement.\n");
    // }

    sqlite3_finalize(stmt);

    *response = malloc(BUF_SIZE);
    snprintf(*response, BUF_SIZE, "Movie added successfully!\n");

    return rc;
}

// Add genre to movie with specified id, if the movie exists
int addGenreToMovie(sqlite3 *db, int id, const char *genre, char **response){
    sqlite3_stmt *stmt;
    int rc;
    char sql[BUF_SIZE];

    // Verify if the movie exists
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM movies WHERE id = %d;", id);
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *response = malloc(BUF_SIZE);
        snprintf(*response, BUF_SIZE, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        *response = malloc(BUF_SIZE);
        snprintf(*response, BUF_SIZE, "Error checking movie existence: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return rc;
    }
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (count == 0) {
        *response = malloc(BUF_SIZE);
        snprintf(*response, BUF_SIZE, "Movie not found: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // Add the genre to the genres field
    snprintf(sql, sizeof(sql),
             "UPDATE movies SET genres = "
             "CASE WHEN genres IS NULL OR genres = '' THEN ? "
             "ELSE genres || ',' || ? END "
             "WHERE id = %d;",
             id);

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *response = malloc(BUF_SIZE);
        snprintf(*response, BUF_SIZE, "Error preparing stmt: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    sqlite3_bind_text(stmt, 1, genre, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, genre, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        *response = malloc(BUF_SIZE);
        snprintf(*response, BUF_SIZE, "Some error occured: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return rc;
    }

    sqlite3_finalize(stmt);

    *response = malloc(BUF_SIZE);
    snprintf(*response, BUF_SIZE, "Genre added sucesfully!\n");

    return rc;
}

// Removes movie with specified id
int removeMovie(sqlite3 *db, int id, char **response){
    sqlite3_stmt *stmt;
    int rc;
    const char *sql = "DELETE FROM movies WHERE id = ?;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *response = malloc(BUF_SIZE);
        snprintf(*response, BUF_SIZE, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        *response = malloc(BUF_SIZE);
        snprintf(*response, BUF_SIZE, "Failed to remove movie: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // char *expanded_sql = sqlite3_expanded_sql(stmt);
    // if (expanded_sql) {
    //     printf("Expanded SQL: %s\n", expanded_sql);
    //     sqlite3_free(expanded_sql);
    // } else {
    //     printf("Failed to expand SQL statement.\n");
    // }

    sqlite3_finalize(stmt);

    *response = malloc(BUF_SIZE);
    snprintf(*response, BUF_SIZE, "Movie removed successfully!\n");

    return rc;
}

// print entire database
int listAll(sqlite3 *db, int fd){
    char *errMsg = 0;
    int rc;
    const char *sql = "SELECT * FROM movies;";

    rc = sqlite3_exec(db, sql, callback, &fd, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    return rc;
}

// list entire database
int listAllMovies(sqlite3 *db, int fd){
    char *errMsg = 0;
    int rc;
    const char *sql = "SELECT id, title FROM movies;";

    rc = sqlite3_exec(db, sql, callback, &fd, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    return rc;
}

// list all movies alongside their ids
int listMovie(sqlite3 *db, int id, int fd){
    char *errMsg = 0; 
    int rc;            
    char sql[BUF_SIZE];    

    snprintf(sql, sizeof(sql), "SELECT * FROM movies WHERE id = %d;", id);

    rc = sqlite3_exec(db, sql, callback, &fd, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        char *msg = "Error executing query\n";
        send(fd, msg, strlen(msg), 0);
        return rc;
    }

    return rc;
}

// list all movies with the specified genre
int listMoviesByGenre(sqlite3 *db, const char *genre, int fd){
    char *errMsg = 0;
    int rc;
    char sql[BUF_SIZE];  
    char *lower_genre; 

    // Convert genre to lowercase for better matching
    size_t genre_len = strlen(genre);
    lower_genre = malloc(genre_len + 1);
    if (!lower_genre) {
        char *msg = "Error: Memory allocation failed\n";
        send(fd, msg, strlen(msg), 0);
        return SQLITE_NOMEM;
    }
    for (size_t i = 0; i < genre_len; i++) {
        lower_genre[i] = tolower((unsigned char)genre[i]);
    }
    lower_genre[genre_len] = '\0';

    // Select the movies with specified genre
    snprintf(sql, sizeof(sql),
             "SELECT * FROM movies WHERE LOWER(',' || genres || ',') LIKE '%%,%s,%%';",
             lower_genre);

    rc = sqlite3_exec(db, sql, callback, &fd, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        char *msg = "Error executing query\n";
        send(fd, msg, strlen(msg), 0);
        free(lower_genre);
        return rc;
    }

    free(lower_genre);
    return rc;
}

// function that is executed in each thread
void *client_handler(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    char buffer[BUF_SIZE];
    int bytes_received;

    // loop for commands
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("error ocurred in recv\n");
            break;
        }
        buffer[bytes_received] = '\0';

        // split the request
        char *args[10];
        int argc = split_request(buffer, args, MAX_ARGS);
        if (argc <= 0)
            continue;

        // printf("command %s with %d args: ", args[0], argc);
        // for (int i = 1; i < argc; i++) {
        //     printf("arg[%d] is %s ", i, args[i]);
        // }
        // printf("\n");

        // add and remove functions have a response argument for printing in the client
        // if the operation was sucess
        if (strcmp(args[0], "ADDMOVIE") == 0 && argc == 5) {
            char *response;
            addMovie(db, args[1], args[2], args[3], args[4], &response);
            send(client_fd, response, strlen(response), 0);
            free(response); 
        } else if (strcmp(args[0], "ADDGENRE") == 0 && argc == 3) {
            char *response;
            addGenreToMovie(db, atoi(args[1]), args[2], &response);
            send(client_fd, response, strlen(response), 0);
            free(response); 
        } else if (strcmp(args[0], "REMOVE") == 0 && argc == 2) {
            char *response;
            removeMovie(db, atoi(args[1]), &response);
            send(client_fd, response, strlen(response), 0);
            free(response);
        } else if (strcmp(args[0], "LIST") == 0 ) {
            if (strcmp(args[1], "ALL" ) == 0 && argc == 2){
                listAll(db, client_fd);
            } else if (strcmp(args[1], "MOVIES") == 0 && argc == 2){
                listAllMovies(db, client_fd);
            } else if (strcmp(args[1], "ID") == 0 && argc == 3) {
                listMovie(db, atoi(args[2]), client_fd);
            } else if (strcmp(args[1], "GENRE") == 0 && argc == 3) {
                listMoviesByGenre(db, args[2], client_fd);
            }
        } else {
            send(client_fd, "Something strange happened\n", 27, 0);
        }
        // marks end of response
        send(client_fd, "\nEND_OF_RESPONSE\n", strlen("\nEND_OF_RESPONSE\n"), 0);
    }
    close(client_fd);
    // printf("Conection close with client :/\n");
    return NULL;
}


int main(){
    // Database variables
    char *errMsg = 0;
    int rc;
    const char *sql;

    // Networking variables
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;
    int status;

    // Open DB and initialize table
    rc = sqlite3_open("test.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open db :( : %s\n", sqlite3_errmsg(db));
        return 1;
    }
    printf("Database opened!\n");

    sql = "CREATE TABLE IF NOT EXISTS movies("
          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
          "title TEXT NOT NULL UNIQUE,"
          "genres TEXT,"
          "director TEXT NOT NULL,"
          "debutDate DATE);";
    
    rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else {printf("Table is ready!\n");} 

    // Networking stuff
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, MYPORT, &hints, &res); 
    if (status != 0){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    status = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (status != 0) {
        fprintf(stderr, "bind error: %s\n", strerror(errno));
        return 1;
    }

    status = listen(sockfd, BACKLOG);
    if (status != 0) {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        return 1;
    }
    
    printf("Server listening on port %s\n", MYPORT);

    for (;;) {
        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) {
            fprintf(stderr, "accept error: %s\n", strerror(errno));
            continue;
        }

        printf("Someone connect to host!\n");

        int *client_fd_ptr = malloc(sizeof(int));
        if (client_fd_ptr == NULL) {
            fprintf(stderr, "malloc error\n");
            close(new_fd);
            continue;
        }
        *client_fd_ptr = new_fd;

        // spawn thread for accepted connection
        pthread_t thread;
        status = pthread_create(&thread, NULL, client_handler, client_fd_ptr);
        if (status != 0) {
            fprintf(stderr, "pthread_create error\n");
            free(client_fd_ptr);
            close(new_fd);
            continue;
        }
        pthread_detach(thread);
    } 

    // Cleanup
    freeaddrinfo(res);
    sqlite3_close(db);
    return 0;
}
