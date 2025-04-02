#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<sqlite3.h>

#define MYPORT "9666"
#define BACKLOG 10

// Callback for db
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    for(int i=0; i<argc; i++){
       printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

// Add movie to the database
int addMovie(sqlite3 *db, const char *sql){
    char *errMsg = 0;
    int rc;
    //sql = "INSERT INTO movies (title, genres, director, debutDate)"
    //      "VALUES ('Get Out', 'Horror,Comedy', 'Jordan Peele', '2017-05-18');";
    rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else {printf("Movie added!\n");}

    return rc;
}

// print entire database
int printDatabase(sqlite3 *db){
    char *errMsg = 0;
    int rc;
    const char *sql = "SELECT * FROM movies;";
    rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    return rc;
}


int main(){
    // Database variables
    sqlite3 *db;
    char *errMsg = 0;
    int rc;
    const char *sql;
    // Networking variables
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;

    rc = sqlite3_open("test.db", &db);
    if (rc) {
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
    } else {printf("Table created!\n");} 

    // Networking stuff
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    rc = getaddrinfo(NULL, MYPORT, &hints, &res); // lidar com rc depois

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    rc = bind(sockfd, res->ai_addr, res->ai_addrlen); // lidar com rc depois
    listen(sockfd, BACKLOG);
    
    addr_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

    // close database
    sqlite3_close(db);
    return 0;
}
