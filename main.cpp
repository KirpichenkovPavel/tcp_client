/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: pavel
 *
 * Created on September 15, 2016, 9:06 PM
 */
#define PSIZE 32
#define FILLER 126
#define PORT 10501
#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <linux/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

using namespace std;

typedef int step_t;

typedef struct package_t{
    char *code;
    char *name;
    char *rate;
    char *change;
} package_t; 

int rok(package_t *spckg);
void freepckg(package_t *p);
int error(int fd, char* mes);
int sendall(int s, char *buf, int len, int flags);
int recvall(int sockfd, void *buf, int len, int flags);
char *get_sum(char *pckg, int size, int offset);
char *get_code(char *pckg);
char *get_name(char *pckg);
char *get_rate(char *pckg);
char *get_change(char *pckg);
package_t *parsepackage(char *pckg);
void get_addr(int &ip, int &port);
char *get_command();
char **splt(char *str);
int disconnect_r(int sock);
int list_r(int sock);
int add_r(int sock, char **args);
int del_r(int sock, char **args);
int set_r(int sock, char **args);
int hist_r(int sock, char **args);
package_t *get_response(int sock);

int error(int fd, char* mes) {
    printf("%s\n%s\n", mes, strerror(errno));
    if (fd) close(fd);
    return -1;
}

int sendall(int s, char *buf, int len, int flags) {
    int total = 0;
    int n;

    while (total < len) {
        n = send(s, buf + total, len - total, flags);
        if (n < 1) {
            break;
        }
        total += n;
    }
    return (n < 1 ? -1 : total);
}

int recvall(int sockfd, void *buf, int len, int flags) {
    int total = 0;
    int n;

    while (total < len) {
        n = recv(sockfd, buf + total, len - total, flags);
        if (n < 1) {
            break;
        }
        total += n;
    }
    return (n < 1 ? -1 : total);
}

char *get_sum(char *pckg, int size, int offset){
    char *_ret = new char[size+1];    
    bzero(_ret, size+1);
    int i;
    for (i=0; i<size; i++){
        if(pckg[i+offset] == FILLER) break;
    }
    strncpy(_ret,pckg+offset,i);
    _ret[i+1]='\0';
    return _ret;
}

char *get_code(char *pckg){
    return get_sum(pckg, 1, 0);
}

char *get_name(char *pckg){
    return get_sum(pckg, 10, 1);
}

char *get_rate(char *pckg){
    return get_sum(pckg, 10, 11);
}

char *get_change(char *pckg){
    return get_sum(pckg, 11, 21);
}

package_t *parsepackage(char *pckg){
    package_t *ret = new package_t;
    ret->code = get_code(pckg);
    ret->name = get_name(pckg);    
    ret->rate = get_rate(pckg);
    ret->change = get_change(pckg);
    return ret;
}

void get_addr(int &ip, int &port){
    
    char *ip_ = new char[17], *port_ = new char[7];
    bzero(ip_, 17);     
    printf("Enter server ip.\n");
    //scanf("%s", ip_);
    fgets(ip_, 16, stdin);
    printf("Enter server port.\n");
    //scanf("%s", port_);
    fgets(port_, 6, stdin);
    port = atoi(port_);    
    ip = inet_addr(ip_);
    delete[] ip_;
    delete[] port_;
    return;
}

char *get_command(){
    
    char* msg = new char[128];
    bzero(msg, sizeof(msg));
    printf("Enter your command$ ");
    while(!strcmp(fgets(msg, 128, stdin),"\n"));
    return msg;
}

char **splt(char *str){
    char **_ret = new char*[3];
    int i;
    for (i=0; i<3; i++){
        _ret[i] = new char[32];
    }
    i = 0;
    for (char *p = strtok(str," \n"); p != NULL; p = strtok(NULL, " \n"))
    {
        if (i == 3) break;
        strcpy(_ret[i++],p);        
    }
    return _ret;        
}

int disconnect_r(int sock){
    int status;
    char *pckg = new char[PSIZE+1];
    memset(pckg, FILLER, PSIZE);
    pckg[0] = '0';
    status = sendall(sock, pckg, PSIZE, NULL);
    delete[] pckg;
    if (status < 0) return error(sock, "Send error.");
    return recvall(sock, pckg, PSIZE, NULL);
    
    //if (status < 0) error(client_socket, "Receive error.");
}

int list_r(int sock){
    int status = 0, i=0;
    package_t *p;
    char *pckg = new char[PSIZE+1];
    memset(pckg, FILLER, PSIZE);
    pckg[0] = '1';
    status = sendall(sock, pckg, PSIZE, NULL);
    if (status < 0){
        delete[] pckg;
        return error(sock, "Send error\n");
    }
    bzero(pckg, PSIZE);
    status = 0;
    status = recvall(sock, pckg, PSIZE, NULL);    
    if (status < 0){
        delete[] pckg;
        return error(sock, "Receive error\n");
    }    
    printf("#### Currencies ####\n");
    p = parsepackage(pckg);
    while(!rok(p)){
        printf("%d)",++i);
        p = parsepackage(pckg);
        printf("Name:%s Rate:%d Change:%s\n",p->name, atoi(p->rate), 
                atoi(p->change)==INT32_MIN ? "NA" : p->change);
        status = recvall(sock, pckg, PSIZE, NULL);
        freepckg(p);
        p = parsepackage(pckg);
        if (status < 0){
            delete[] pckg;            
            return error(sock, "Receive error\n");
        }
    }    
    printf("#### End ####\n");
    delete[] pckg;
    freepckg(p);
    return 0;
}

int add_r(int sock, char **args){    
    int status;
    if (strcmp(args[1],"")==0){
        printf("No name provided\n");
        return 0;
    }
    if (strlen(args[1])>10){
        printf("Name is too long\n");
        return 0;
    }        
    char *pckg = new char[PSIZE+1];
    char *name = new char[11];
    package_t *pckg_struct;
    strcpy(name, args[1]);
    memset(pckg, FILLER, PSIZE);
    strcpy(pckg+1, name);
    pckg[0] = '2';
    status = sendall(sock, pckg, PSIZE, NULL);    
    if (status < 0) {
        delete[] pckg;
        return error(sock, "Send error.\n");
    }
    status = recvall(sock, pckg, PSIZE, NULL);
    if (status < 0) {
        delete[] pckg;
        return error(sock, "Receive error.\n");
    }
    pckg_struct = parsepackage(pckg);
    if (rok(pckg_struct)==0){
        delete[] pckg;
        freepckg(pckg_struct);
        error(NULL,
                "Server returned negative answer. Currency was not added.\n");
        return 0;
    }
    printf("Currency was added.\n");
    delete[] pckg;
    freepckg(pckg_struct);
    return 0;
}

int del_r(int sock, char **args){
    int status;
    if (strcmp(args[1],"")==0){
        printf("No name provided\n");
        return 0;
    }        
    if (strlen(args[1])>10){
        printf("Name is too long\n");
        return 0;
    }
        
    char *pckg = new char[PSIZE+1];    
    package_t *pckg_struct;    
    memset(pckg, FILLER, PSIZE);
    strcpy(pckg+1, args[1]);
    pckg[0] = '3';
    status = sendall(sock, pckg, PSIZE, NULL);    
    if (status < 0) {
        delete[] pckg;
        return error(sock, "Send error.\n");
    }
    status = recvall(sock, pckg, PSIZE, NULL);
    if (status < 0) {
        delete[] pckg;
        return error(sock, "Receive error.\n");
    }
    pckg_struct = parsepackage(pckg);
    if (rok(pckg_struct)==0){
        delete[] pckg;
        freepckg(pckg_struct);
        error(NULL,
                "Server returned negative answer. Currency was not deleted.\n");
        return 0;
    }
    printf("Currency was deleted.\n");
    delete[] pckg;
    freepckg(pckg_struct);
    return 0;    
}

int set_r(int sock, char **args){
    int status;
    if (strcmp(args[1],"")==0){
        printf("No name provided\n");
        return 0;
    }
    if (strlen(args[1])>10){
        printf("Name is too long\n");
        return 0;
    }
    if (strcmp(args[2],"")==0){
        printf("No rate provided\n");
        return 0;
    }
    if (strlen(args[1])>10 || atoi(args[1])<0){
        printf("Incorrect rate\n");
        return 0;
    }    
    char *pckg = new char[PSIZE+1];
    package_t *pckg_struct;
    memset(pckg, FILLER, PSIZE);
    strncpy(pckg+1, args[1], strlen(args[1]));
    strncpy(pckg+11, args[2], strlen(args[2]));
    pckg[0] = '4';
    status = sendall(sock, pckg, PSIZE, NULL);    
    if (status < 0) {
        delete[] pckg;
        return error(sock, "Send error.\n");
    }
    status = recvall(sock, pckg, PSIZE, NULL);
    if (status < 0) {
        delete[] pckg;
        return error(sock, "Receive error.\n");
    }
    pckg_struct = parsepackage(pckg);
    if (rok(pckg_struct)==0){
        delete[] pckg;
        freepckg(pckg_struct);
        printf("Server returned negative answer. Currency was not set.\n");
        return 0;
    }
    printf("Currency was set.\n");
    delete[] pckg;
    freepckg(pckg_struct);
    return 0;
}

int hist_r(int sock, char **args){
    int status;
    if (strcmp(args[1],"")==0){
        printf("No name provided\n");
        return 0;
    }
    if (strlen(args[1])>10){
        printf("Name is too long\n");
        return 0;
    }
    char *pckg = new char[PSIZE+1];
    package_t *p;
    memset(pckg, FILLER, PSIZE);
    strncpy(pckg+1, args[1], strlen(args[1]));
    pckg[0] = '5';
    status = sendall(sock, pckg, PSIZE, NULL);    
    if (status < 0) {
        delete[] pckg;
        return error(sock, "Send error.\n");
    }
    bzero(pckg, PSIZE);
    status = recvall(sock, pckg, PSIZE, NULL);    
    if (status < 0){
        delete[] pckg;
        return error(sock, "Receive error\n");
    }    
    int i=0;
    p = parsepackage(pckg);
    if (atoi(p->code)==0){
        freepckg(p);
        delete[] pckg;
        printf("Received error status from server\n");
        return 0;
    }
    if (atoi(p->change)!=0)
        printf("Something went wrong, the following history may be inconsistent.\n");
    printf("#### History for currency <%s> ####\n", p->name);    
    while(!rok(p)){                
        printf("%d)",++i);
        printf("Rate:%d\n", atoi(p->rate));
        status = recvall(sock, pckg, PSIZE, NULL);
        freepckg(p);
        p = parsepackage(pckg);
        if (status < 0){
            delete[] pckg;            
            return error(sock, "Receive error\n");
        }
    }
    printf("#### End of history ####\n");
    delete[] pckg;
    return 0;
}

package_t *get_response(int sock){
    char *pckg = new char[PSIZE+1], *code, *name, *rate, *chg;
    package_t *ret = NULL;
    if (recvall(sock, pckg, PSIZE, 0) < 0){
        error(sock, "Initial message receive error.");
        return ret;
    }
    code = get_code(pckg);
    name = get_name(pckg);
    rate = get_rate(pckg);
    chg = get_change(pckg);
    ret = new package_t;
    ret->code = get_code(pckg);
    ret->name = get_name(pckg);
    ret->rate = get_rate(pckg);
    ret->change = get_change(pckg);
    return ret;
}

int rok(package_t *spckg){
    if (strcmp(spckg->code, "1")==0)
        return 1;
    else
        return 0;
}

void freepckg(package_t *p){
    if(p){
        delete p->name;
        delete p->code;
        delete p->rate;
        delete p->change;
        delete p;
    }    
}

int main(int argc, char** argv) {    
    char **args, *cmd, *msg_buf;    
    int client_socket, ip, port, status;    
    struct sockaddr_in server_address;
    get_addr(ip, port);
    server_address.sin_addr.s_addr = ip;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);    
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    status = connect(client_socket, (struct sockaddr *) &server_address,
            sizeof(server_address));
    if (status < 0) {
        return error(client_socket, ("Connection error."));        
    }
    package_t *spckg = get_response(client_socket);
    if (!(spckg && rok(spckg)))
        return error(client_socket, "Init package error\r\n");
    freepckg(spckg);
    while(1){
        int status = 0;
        msg_buf = get_command();
        args = splt(msg_buf);
        cmd = args[0];
        if(strcmp(cmd,"quit")==0){
            status = disconnect_r(client_socket);
            if(status!=-1)
                printf("Quit request failed, unsafe exit.\r\n");
            break;
        }
        else if(strcmp(cmd,"list")==0)
            status = list_r(client_socket);        
        else if(strcmp(cmd,"add")==0)
            status = add_r(client_socket, args);
        else if(strcmp(cmd,"del")==0)
            status = del_r(client_socket, args);
        else if(strcmp(cmd,"set")==0)
            status = set_r(client_socket, args);
        else if(strcmp(cmd,"hist")==0)
            status = hist_r(client_socket, args);
        else
            printf("Unknown command\r\n");
        if (status!=0) break;
    }    
    for(int i=0; i<3; i++){
        delete args[i];
    }    
    delete[] args;
    delete[] msg_buf;    
    return 0;
}