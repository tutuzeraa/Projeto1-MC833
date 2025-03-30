#include<stdio.h>
#include<sqlite3.h>

int main(){
    sqlite3 *db;
    int rc;

    rc = sqlite3_open("test.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open db :( : %s\n", sqlite3_errmsg(db));
        return 1;
    }

    printf("Deu bom!\n");

    return 0;
}
