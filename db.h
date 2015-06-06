#ifndef DB_H_INCLUDED
#define DB_H_INCLUDED
#include <stdio.h>
#include <sqlite3.h>
#include <cstdio>
#include<iostream>
#include<string>
#include<vector>
using namespace std;
static vector<string> values;
class Database{
    public:
    sqlite3 *db;
    char *zErrMsg;
    int rc;
    char * sql;


    Database(string dbName){
        rc = sqlite3_open(dbName.c_str(),&db);
        if( rc ){
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));

        }
        else
            fprintf(stderr, "Opened database successfully\n");
    }

    static int callback(void *data, int argc, char **argv, char **azColName){

        for(int i=0; i<argc; i++){
            values.push_back(argv[i]);
           // printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        }
        printf("\n");
        return 0;
    }

    bool createTable(string tSql){
        rc = sqlite3_exec(db, tSql.c_str(), NULL, 0, &zErrMsg);
        if( rc != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return false;
        }
        else
            return true;
        }

    bool insertTable(string Sql){
        rc = sqlite3_exec(db, Sql.c_str(), NULL, 0, &zErrMsg);
        if( rc != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return false;
        }
        else
            return true;
    }

    bool selectTable(string Sql){
        char * dummy = "i";
        rc = sqlite3_exec(db, Sql.c_str(), callback, (void*)dummy, &zErrMsg);
        if( rc != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return false;
        }
        else
            return true;
    }

};


#endif // DB_H_INCLUDED
