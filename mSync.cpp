#include <stdio.h>
#include<queue>
#include<fstream>
#include <sqlite3.h>
#include <cstdio>
#include<iostream>
#include<vector>
#include<unistd.h>
#include <dirent.h>
#include<sys/stat.h>
#include "db.h"
#include<cstring>
#include <stdlib.h>
using namespace std;

vector<pair<string,string> > filesDiff;
void getDir(char rootPath[]){
    getcwd(rootPath,sizeof(rootPath));
}

void getRoot(char rootPath[],char currPath[]){
    int i = 0;
    int countSlashes = 0;
    for(;i<strlen(currPath);i++){
        if(countSlashes >= 4)
            break;

        if(currPath[i] == '/')
            countSlashes++;
        rootPath[i] = currPath[i];
    }
    if(rootPath[i-1]!='/')
        rootPath[i++] = '/';

    rootPath[i] = '\0';
}

bool isDirectory(string path){
    struct stat stat_buff;
    stat(path.c_str(),&stat_buff);
    return ((stat_buff.st_mode & S_IFDIR) != 0);
}

void syncPaths(string & target,string & src){
    vector <string> fileList;
    DIR * dir = opendir(src.c_str());
    struct dirent * dBuff;
    if(dir != NULL){
        while(dBuff = readdir(dir)){
            if(dBuff->d_name[strlen(dBuff->d_name)-1] != '~'){
                fileList.push_back(dBuff->d_name);
            }
        }
    }

    struct stat buff_target;
    struct stat buff_src;

    for(int i = 0;i<fileList.size();i++){
        if(strcmp(fileList.at(i).c_str(),".")==0 || strcmp(fileList.at(i).c_str(),".0")==0 || strcmp(fileList.at(i).c_str(),"..")==0)
            continue;

        string tPath = target + "/" + fileList.at(i);
        string sPath = src + "/" + fileList.at(i);
        if(stat(tPath.c_str(),&buff_target) != 0)
        {
            filesDiff.push_back(make_pair(string("'" + target + "'"),string("'" + sPath + "'")));
        }
        else{
            if(buff_target.st_mode & S_IFDIR){
                syncPaths(tPath,sPath);
            }
            else{
                stat((sPath).c_str(),&buff_src);
                if(buff_target.st_mtim.tv_sec < buff_src.st_mtim.tv_sec)
                    filesDiff.push_back(make_pair(string("'" + target + "'"),string("'" + sPath + "'")));
            }
        }

    }



}

void makeChanges(){
    string command;
    for(int i = 0;i<filesDiff.size();i++){
        command = "cp -aur " + filesDiff.at(i).second + " " + filesDiff.at(i).first;
        cout<<"Copying "<<filesDiff.at(i).second<<" ..."<<endl;
        system(command.c_str());
        }
}

void viewChanges(){
    if(filesDiff.size() > 0)
        cout<<"Files to be changed: "<<endl;
    else
        cout<<"No files changed"<<endl;
     for(int i = 0;i<filesDiff.size();i++){
        cout<<filesDiff.at(i).first<<" -- "<<filesDiff.at(i).second<<endl;
    }
}

bool isEmpty(fstream & f){
    if(f.peek() == std::ifstream::traits_type::eof())
        return true;
    return false;
}

void extractFromFile(fstream & statStream){
        statStream.clear();
       char buffer[256];
            queue<string> Q;
            while(!statStream.eof()){
                statStream.getline(buffer,256,'}');
                Q.push(string(buffer));
            }
            cout<<"size: "<<Q.size()<<endl;
            while(Q.size() !=0){
                string first = Q.front();
                Q.pop();
                string second = Q.front();
                Q.pop();
                cout<<first<<" -- "<<second<<endl;
                filesDiff.push_back(make_pair(first,second));
            }
}

void writeDIff(fstream & statStream){
    statStream.clear();
    if(!isEmpty(statStream))
        statStream<<"}";
    statStream.clear();
    for(int i = 0;i<filesDiff.size();i++){
        statStream<<filesDiff.at(i).first<<"}"<<filesDiff.at(i).second;
        if(i != filesDiff.size() - 1)
            statStream<<"}";
    }

    statStream.close();
    if(filesDiff.size())
        cout<<"Difference Written"<<endl;
}

void isInitialized(string & dbPath){
    struct stat buff;
    if(stat(dbPath.c_str(),&buff) != 0)
        cout<<"Run init first"<<endl;exit(0);

}

int main(int argc,char * argv[]){
    extern vector<string>values;
    char currPath[100];
    char rootPath[100];
    struct stat st_buff;
    Database * dirDb;
    getcwd(currPath,sizeof(currPath));
    getRoot(rootPath,currPath);
    string dbPath = string(rootPath) + "syncDB.db";
    if(stat(dbPath.c_str(),&st_buff) == 0)
        dirDb = new Database(dbPath);
    string path = string(rootPath) + "status.sync";
    fstream statStream(path.c_str(),fstream::in | fstream::out | fstream::app);
    if(argv[1] == NULL)
        cout<<"No command line arguments"<<endl;
    else if(strcmp(argv[1],"init") == 0){
        dirDb = new Database(dbPath);
        if(stat(dbPath.c_str(),&st_buff) != 0){
            cout<<"Initializing for the first time.."<<endl;
        }
        else
            cout<<"Already Initialized.."<<endl;

        cout<<dirDb->createTable("CREATE TABLE IF NOT EXISTS syncedDir (removable_dsk_path TEXT,local_path TEXT,PRIMARY KEY(removable_dsk_path,local_path))");

    }
    else if(strcmp(argv[1],"sync") == 0 && argv[2] != NULL && argv[3] != NULL){
        isInitialized(dbPath);
        string src = string(argv[3]);
        string target = string(currPath) + "/" + string(argv[2]);
        if(!(isDirectory(src) && isDirectory(target))){
            cout<<"These Paths are not directory"<<endl;
            return 0;
        }
        string sql = "INSERT INTO syncedDir (removable_dsk_path,local_path) VALUES ('" + target + "','" + src + "')";
        if(!dirDb->insertTable(sql))
            return 0;
        syncPaths(target,src);
        writeDIff(statStream);
        viewChanges();
    }
    else if(strcmp(argv[1],"update") == 0){
        isInitialized(dbPath);
        if(isEmpty(statStream)){
            cout<<"Run status command first"<<endl;
            return 0;
        }
        if(strcmp(argv[2],"all") == 0){
            extractFromFile(statStream);
            makeChanges();
            remove(path.c_str());
        }
        else{
            struct stat st_buff;
            if(argv[2][strlen(argv[2])-1] == '/')
                return 0;

            if(stat(argv[2],&st_buff) == 0){
                string sPath = string(argv[2]);
                string tPath = sPath.substr(0,sPath.find_last_of("/"));
                string command = "cp -aur " + sPath + " " + tPath;
                //cout<<sPath<<endl<<tPath<<endl;
                system(command.c_str());
            }
            else
                return 0;
        }
    }
    else if(strcmp(argv[1],"status") == 0){
        isInitialized(dbPath);
        if(!isEmpty(statStream)){
            extractFromFile(statStream);
            viewChanges();
        }
        else{
            dirDb->selectTable("SELECT * FROM syncedDir");
            for(int i = 0;i<values.size();i++){
                syncPaths(values.at(i),values.at(i+1));
                i++;
            }
            writeDIff(statStream);
            viewChanges();
        }
    }
    else if(strcmp(argv[1],"view")==0){
        dirDb->selectTable("SELECT * FROM syncedDir");
        cout<<"Synced Paths"<<endl;
        for(int i = 0;i<values.size();i++){
            cout<<values.at(i)<<" -- " <<values.at(i+1)<<endl;
            i++;
        }
    }
    else if(strcmp(argv[1],"remove") == 0 && argv[2] != NULL && argv[3] != NULL){
        string removablePath = string(argv[2]);
        string localPath = string(argv[3]);
        string sql = "DELETE FROM syncedDir where removable_dsk_path = '" + removablePath + "' and local_path = '" + localPath + "'";
        if(dirDb->selectTable(sql))
            cout<<"Removal complete"<<endl;
        else
            cout<<"Error Occured"<<endl;
    }
    else{
        cout<<"Invalid command"<<endl;
    }
    return 0;
}
