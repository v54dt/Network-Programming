
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <time.h>
#include <vector>

using namespace std;

struct clientInfo
{
    int fd;
    sockaddr_in addr;
    char ip[20];
    unsigned short port;
    string name;
    string pos;

    clientInfo() : name("anonymous"), fd(-1), pos("lobby") {}
};

struct post_row
{
    //ID,TITLE,AUTHOR,DATE
    int ID;
    string BOARD;
    string TITLE;
    string AUTHOR;
    string DATE;
    string CONTENT;
    string COMMENT;
};

struct board_row
{
    int INDEX;
    string NAME;
    string MODERATOR;
};

clientInfo clientInfo_array[30];
sqlite3 *db;
char *zErrMsg = 0;
int rc;
string sql;

string password;
string sendback_message = "";
string list_post = "";
// 取得 sockaddr，IPv4 或 IPv6：
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

static int callback(void *data, int argc, char **argv, char **azColName)
{
    int i;
    // fprintf(stderr, "%s: \n", (const char *)data);
    for (i = 0; i < argc; i++)
    {
        //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
}
/*
int callback_count(void *data, int argc, char **argv, char **azColName)
{
    cout << "argv[0]" << argv[0] << endl;
    cout << "atoi" << atoi(argv[0]) << endl;

    return atoi(argv[0]);
}
*/
/*
string callback_author(void *data, int argc, char **argv, char **azColName)
{
    cout << "AUTHOR :" << argv[0] << endl;
    string ret(argv[0]);
    return ret;
}
*/
static int callback_password(void *data, int argc, char **argv, char **azColName)
{
    int i;
    // fprintf(stderr, "%s: \n", (const char *)data);
    for (i = 0; i < argc; i++)
    {
        //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    // printf("");
    password.assign(argv[1]);

    return 0;
}
static int callback_sendback_message(void *data, int argc, char **argv, char **azColName)
{
    for (int i = 0; i < argc; i++)
    {
        sendback_message += "\t";
        sendback_message += argv[i];
    }
    //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    // printf("");
    sendback_message += "\n";
    return 0;
}
static int callback_list_post(void *data, int argc, char **argv, char **azColName)
{
    cout << "argc:" << argc << endl;
    static string s_argv[4];
    for (int i = 0; i < argc; i++)
        s_argv[i].assign(argv[i]);
    //cout << s_argv[0] << endl;
    //cout << s_argv[1] << endl;
    //cout << s_argv[2] << endl;
    //cout << s_argv[3] << endl;
    //printf("\n");
    char tmp[500];
    sprintf(tmp, "%10s%10s%10s%10s", argv[0], argv[1], argv[2], argv[3]);

    printf("tmp is :%s\n", tmp);

    if (s_argv[0] == "" || s_argv[1] == "")
        cout << "WHAT THE FUCK" << endl;

    //cout << s_argv[0] << endl
    //     << s_argv[1] << s_argv[2] << s_argv[3] << endl;
    //printf("%s%s%s%s\n", s_argv[0].c_str(), s_argv[1].c_str(), s_argv[2].c_str(), s_argv[3].c_str());
    //list_post = list_post + "\tID:" + s_argv[0] + "\tTitle:" + s_argv[1] + "\tAuthor:" + s_argv[2] + "\tDate:" + s_argv[3] + "\n";
    //list_post = list_post + "\t" + s_argv[0] + "\t" + s_argv[1] + "\t" + s_argv[2] + "\t" + s_argv[3] + "\n";
    //cout << "list_post callback\n"
    //    << list_post << endl;
    return 0;
}
static int callback_read(void *data, int argc, char **argv, char **azColName)
{
    // fprintf(stderr, "%s: \n", (const char *)data);
    /*for (int i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
*/

    string s_argv[4];
    for (int i = 0; i < argc; i++)
        s_argv[i].assign(argv[i]);

    sendback_message = "";
    sendback_message = "\tAuthor\t:" + s_argv[0] + "\n\tTitle:\t" + s_argv[1] + "\n\tDate:\t" + s_argv[2] + "\n\t--\n\t" + s_argv[3] + "\t--\n";
    return 0;
}

void create_database()
{
    rc = sqlite3_open("bbs.db", &db);
    if (rc)
    {
        // fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }
    else
    {
        // fprintf(stderr, "Opened database successfully\n");
        string sql1 =
            "CREATE TABLE ACCOUNTS("
            "ID CHAR(50) PRIMARY KEY NOT NULL,"
            "PASSWORD CHAR(50) NOT NULL);";

        rc = sqlite3_exec(db, sql1.c_str(), callback, 0, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            //fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            // fprintf(stdout, "Table created successfully\n");
        }
        string sql2;
        sql2 =
            "CREATE TABLE BOARD("
            "`INDEX` INTEGER PRIMARY KEY AUTOINCREMENT,"
            "NAME CHAR(50) NOT NULL UNIQUE,"
            "MODERATOR CHAR(50) NOT NULL);";

        rc = sqlite3_exec(db, sql2.c_str(), callback, 0, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            //fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            //fprintf(stdout, "Table Board created successfully\n");
        }
        string sql3;
        sql3 =
            "CREATE TABLE POST("
            "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
            "BOARD CHAR(50) NOT NULL,"
            "AUTHOR CHAR(50) NOT NULL,"
            "`DATE` REAL NOT NULL,"
            "TITLE TEXT NOT NULL,"
            "CONTENT TEXT,"
            "COMMENT TEXT"
            ");";

        //TITLE CHAR(50) NOT NULL,"
        //                         "AUTHOR CHAR(50) NOT NULL,"
        //                        "DATE DATE

        rc = sqlite3_exec(db, sql3.c_str(), callback, 0, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            //fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            //fprintf(stdout, "Table POST created successfully\n");
        }

        /*string sq4;
        sq4 =
            "CREATE TABLE ARTICLES("
            "ID INTEGER NOT NULL,"
            "TITLE TEXT NOT NULL,"
            "CONTENT TEXT,"
            "COMMENT TEXT,"
            "FOREIGN KEY(ID) REFERENCES POST(ID)"
            ");";

        rc = sqlite3_exec(db, sql4.c_str(), callback, 0, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            fprintf(stdout, "Table ARTICLES created successfully\n");
        }*/
    }
}

void ReplaceStringInPlace(std::string &subject, const std::string &search,
                          const std::string &replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos)
    {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}
int main(int argc, char **argv)
{
    fd_set master;   // master file descriptor 清單
    fd_set read_fds; // 給 select() 用的暫時 file descriptor 清單
    int fdmax;       // 最大的 file descriptor 數目

    // const char *bindport = argv[0];

    int listener;                       // listening socket descriptor
    int newfd;                          // 新接受的 accept() socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256]; // 儲存 client 資料的緩衝區
    memset(buf, 0, sizeof(buf));

    const char *data = "Callback function called";

    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes = 1; // 供底下的 setsockopt() 設定 SO_REUSEADDR
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master); // 清除 master 與 temp sets
    FD_ZERO(&read_fds);

    // 給我們一個 socket，並且 bind 它
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &ai)) != 0)
    {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }

        // 避開這個錯誤訊息："address already in use"
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }

        break;
    }

    // 若我們進入這個判斷式，則表示我們 bind() 失敗
    if (p == NULL)
    {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1)
    {
        perror("listen");
        exit(3);
    }

    // 將 listener 新增到 master set
    FD_SET(listener, &master);

    // 持續追蹤最大的 file descriptor
    fdmax = listener; // 到此為止，就是它了
    create_database();
    // 主要迴圈
    for (;;)
    {
        read_fds = master; // 複製 master

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }

        // 在現存的連線中尋找需要讀取的資料
        for (i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            { // 我們找到一個！！
                if (i == listener)
                {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

                    if (newfd == -1)
                    {
                        perror("accept");
                    }
                    else
                    {
                        FD_SET(newfd, &master); // 新增到 master set
                        if (newfd > fdmax)
                        { // 持續追蹤最大的 fd
                            fdmax = newfd;
                        }
                        printf("New Connection\n");
                        char mes[] =
                            "******************************\n"
                            "**Welcome to the BBS server.**\n"
                            "******************************\n";
                        //cout << mes << endl;
                        send(newfd, mes, sizeof(mes), 0);

                        /*printf("New Connection from %s on ""socket %d\n",inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr *)&remoteaddr), remoteIP,INET6_ADDRSTRLEN),newfd);*/
                    }
                }
                else
                {
                    // 處理來自 client 的資料
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
                    {
                        // got error or connection closed by client
                        if (nbytes == 0)
                        {
                            // 關閉連線
                            printf("socket %d hung up\n", i);
                            clientInfo_array[i].name = "anonymous";
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(i);           // bye!
                        FD_CLR(i, &master); // 從 master set 中移除
                    }
                    else
                    {
                        // 我們從 client 收到一些資料
                        // printf("Input is : %s", buf);

                        stringstream ss;
                        ss << buf;
                        /*string command_prompt;
                        ss >> command_prompt;

                        // cout << "command_prompt is:" << command_prompt << endl;
                        if (command_prompt.compare(0, 1, "%") != 0)
                        {
                            char mes[] = "Please input % first\n";
                            send(i, mes, sizeof(mes), 0);
                            continue;
                        }*/

                        string command;
                        ss >> command;

                        string s[5];

                        if (command.compare("register") == 0)
                        {
                            int k = 0;
                            while (ss >> s[k])
                            {
                                k++;
                            }
                            //cout << "k:" << k << endl;
                            if (k != 3)
                            {
                                char mes[] = "Usage: register <username> <email> <password>\n";
                                send(i, mes, sizeof(mes), 0);
                            }
                            else
                            {
                                sql =
                                    "INSERT INTO ACCOUNTS(ID,PASSWORD) "
                                    "VALUES ('" +
                                    s[0] + "','" + s[2] + "');";
                                rc = sqlite3_exec(db, sql.c_str(), callback_password, 0, &zErrMsg);
                                if (rc != SQLITE_OK)
                                {
                                    // string mes = s[0] + " has registered!\n";
                                    // cout << mes.c_str() << endl;
                                    // cout << sizeof(mes.c_str()) << endl;
                                    // cout << mes.length()<<endl;
                                    // send(i, mes.c_str(), mes.length(), 0);
                                    char mes[] = "Username is already used.\n";
                                    send(i, mes, sizeof(mes), 0);
                                }
                                else
                                {
                                    char mes[] = "Register Successfully\n";
                                    send(i, mes, sizeof(mes), 0);
                                }
                            }
                        }
                        else if (command.compare("login") == 0)
                        {
                            int k = 0;
                            while (ss >> s[k])
                            {
                                k++;
                            }
                            // cout << "k:" << k << endl;
                            if (clientInfo_array[i].name != "anonymous")
                            {
                                char mes[] = "Please logout first.\n";
                                send(i, mes, sizeof(mes), 0);
                            }
                            else
                            {
                                if (k != 2)
                                {
                                    char mes[] = "Usage: login <username> <password>\n";
                                    send(i, mes, sizeof(mes), 0);
                                }
                                else
                                {
                                    sql =
                                        "SELECT * from ACCOUNTS "
                                        "WHERE ID = '" +
                                        s[0] + "';";

                                    /* Execute SQL statement */
                                    rc = sqlite3_exec(db, sql.c_str(), callback_password, (void *)data,
                                                      &zErrMsg);
                                    if (rc != SQLITE_OK)
                                    {
                                        fprintf(stderr, "SQL error: %s", zErrMsg);
                                        sqlite3_free(zErrMsg);
                                    }
                                    else
                                    {
                                        // fprintf(stdout, "Operation done successfully");
                                        // cout << "Password is "<< password<<endl;

                                        if (s[1].compare(password) == 0)
                                        {
                                            clientInfo_array[i].name = s[0];

                                            string mes = "Welcome, " + s[0] + ".\n";

                                            // char mes[] = "Loging Succefully\n";
                                            send(i, mes.c_str(), mes.length(), 0);
                                        }
                                        else
                                        {
                                            char mes[] = "Login Failed\n";
                                            send(i, mes, sizeof(mes), 0);
                                        }
                                    }
                                }
                            }
                        }
                        else if (command.compare("logout") == 0)
                        {
                            if (clientInfo_array[i].name == "anonymous")
                            {
                                char mes[] = "Please login first.\n";
                                send(i, mes, sizeof(mes), 0);
                            }
                            else
                            {
                                string mes = "Bye, " + clientInfo_array[i].name + ".\n";
                                send(i, mes.c_str(), mes.length(), 0);

                                clientInfo_array[i].name = "anonymous";
                            }
                        }
                        else if (command.compare("whoami") == 0)
                        {
                            // cout << i << endl;
                            if (clientInfo_array[i].name != "anonymous")
                            {
                                string mes = clientInfo_array[i].name + "\n";
                                send(i, mes.c_str(), mes.length(), 0);
                            }
                            else
                            {
                                char mes[] = "Please login first.\n";
                                send(i, mes, sizeof(mes), 0);
                            }
                        }
                        else if (command.compare("exit") == 0)
                        {
                            close(i);
                            FD_CLR(i, &master);
                        }
                        else if (command.compare("check") == 0)
                        {
                            sql = "SELECT * from ACCOUNTS ;";

                            /* Execute SQL statement */
                            rc = sqlite3_exec(db, sql.c_str(), callback, (void *)data, &zErrMsg);
                            if (rc != SQLITE_OK)
                            {
                                fprintf(stderr, "SQL error: %s", zErrMsg);
                                sqlite3_free(zErrMsg);
                            }
                            else
                            {
                                fprintf(stdout, "Operation done successfully");
                            }
                        }
                        else if (command.compare("create-board") == 0)
                        {
                            if (clientInfo_array[i].name == "anonymous")
                            {
                                char mes[] = "Please login first.\n";
                                send(i, mes, sizeof(mes), 0);
                            }
                            else
                            {
                                int k = 0;
                                ss >> s[k];

                                sql = "INSERT INTO BOARD(NAME,MODERATOR)"
                                      "VALUES (\042" +
                                      s[k] + "\042 , \042" + clientInfo_array[i].name + "\042);";
                                //cout << sql.c_str() << endl;
                                rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
                                if (rc != SQLITE_OK)
                                {
                                    char mes[] = "Board is already exist.\n";
                                    send(i, mes, sizeof(mes), 0);
                                }
                                else
                                {
                                    char mes[] = "Create Board successfully.\n";
                                    send(i, mes, sizeof(mes), 0);
                                }
                            }
                        }
                        else if (command.compare("create-post") == 0)
                        {
                            /*string board_name, command_title, title = "", tmp = "", command_content, content;
                            ss >> board_name;
                            ss >> command_title;
                            ss >> title;
                            ss >> tmp;

                            while (tmp != "--content")
                            {
                                title = title + " " + tmp;
                                ss >> tmp;
                            }
                            command_content = tmp;
                            */
                            if (clientInfo_array[i].name == "anonymous")
                            {
                                char mes[] = "Please login first.\n";
                                send(i, mes, sizeof(mes), 0);
                            }
                            else
                            {
                                string board_name, title, content;
                                ss >> board_name;

                                string line(buf);
                                int title_pos, content_pos;
                                title_pos = line.find("--title");
                                content_pos = line.find("--content");
                                int title_length, content_length;
                                title_length = content_pos - title_pos - 9;
                                content_length = line.length() - 2 - content_pos - 10;
                                title = line.substr(title_pos + 8, title_length);
                                content = line.substr(content_pos + 10, content_length);

                                sqlite3_stmt *stmt3;
                                sql = "SELECT COUNT(*) FROM  BOARD  WHERE NAME = \042" + board_name + "\042;";
                                rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt3, NULL);

                                int match_lines;

                                while ((rc = sqlite3_step(stmt3)) == SQLITE_ROW)
                                {
                                    match_lines = sqlite3_column_int(stmt3, 0);
                                    //cout << "match_lines: " << match_lines << endl;
                                }
                                sqlite3_finalize(stmt3);

                                if (match_lines)
                                {
                                    time_t rawtime;
                                    struct tm *timeinfo;

                                    time(&rawtime);
                                    timeinfo = localtime(&rawtime);
                                    char time_buf[10];
                                    strftime(time_buf, 10, "%m/%d", timeinfo);
                                    string date(time_buf);

                                    //sql = "INSERT INTO POST(BOARD,AUTHOR,DATE,TITLE,CONTENT) VALUES(\042" + board_name + "\042 , \042" + clientInfo_array[i].name + "\042,\042" + time_buf + "\042,\042" + title + "\042,\042" + content + "\042);";

                                    sql = "INSERT INTO POST(BOARD,AUTHOR,DATE,TITLE,CONTENT) VALUES(\042" + board_name + "\042 , \042" + clientInfo_array[i].name + "\042,datetime('now', 'localtime'),\042" + title + "\042,\042" + content + "\042);";

                                    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);

                                    if (rc != SQLITE_OK)
                                    {
                                        fprintf(stderr, "SQL error: %s", zErrMsg);
                                        sqlite3_free(zErrMsg);
                                        char mes[] = "Create post failed.\n";
                                        send(i, mes, sizeof(mes), 0);
                                    }
                                    else
                                    {
                                        char mes[] = "Create post successfully.\n";
                                        send(i, mes, sizeof(mes), 0);
                                    }
                                }
                                else
                                {
                                    char mes[] = "Board does not exist.\n";
                                    send(i, mes, sizeof(mes), 0);
                                }

                                /*
                                cout << "title: " << title << endl;
                                cout << "title_length:" << title_length << endl;
                                cout << "title.length():" << title.length() << endl;

                                cout << "content: " << content << endl;
                                cout << "content_length" << content_length << endl;
                                cout << "content.length():" << content.length() << endl;
*/
                            }
                        }
                        else if (command.compare("list-board") == 0)
                        {
                            string query;
                            sendback_message = "\tIndex\tName\tModerator\n";
                            if ((ss.rdbuf()->in_avail() - 2) != 0) // check advanced query
                            {
                                ss >> query;
                                query.erase(std::remove(query.begin(), query.end(), '#'), query.end());

                                sql = "SELECT * FROM BOARD WHERE NAME LIKE '%" + query + "%';";
                                sqlite3_stmt *stmt;
                                //sql = "SELECT ID,TITLE,AUTHOR,DATE FROM \042" + board_name + "\042;";
                                rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

                                vector<board_row> board_list;
                                while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                                {
                                    board_row tmp;

                                    tmp.INDEX = sqlite3_column_int(stmt, 0);
                                    tmp.NAME = std::string(strdup((char *)sqlite3_column_text(stmt, 1)));
                                    tmp.MODERATOR = std::string(strdup((char *)sqlite3_column_text(stmt, 2)));

                                    board_list.push_back(tmp);
                                }
                                sqlite3_finalize(stmt);

                                string mes = "\n\tIndex\tName\t\tModerator\n";
                                char tmp[1024];
                                for (int j = 0; j < board_list.size(); j++)
                                {
                                    sprintf(tmp, "\t%-8d%-16.16s%-16.16s\n", board_list[j].INDEX, board_list[j].NAME.c_str(), board_list[j].MODERATOR.c_str());
                                    string t;
                                    t.assign(tmp);
                                    mes = mes + t;
                                    //mes = mes + "\t" + post_list[j].ID + "\t" + post_list[j].TITLE + "\t\t" + post_list[j].AUTHOR + "\t\t" + post_list[j].DATE + "\n";
                                }
                                int byte_send;
                                byte_send = send(i, mes.c_str(), mes.length(), 0);
                                //cout << byte_send << endl;
                            }
                            else
                            {
                                sql = "SELECT * FROM BOARD;";
                                //////////////////////////////////////////////////
                                sqlite3_stmt *stmt;
                                //sql = "SELECT ID,TITLE,AUTHOR,DATE FROM \042" + board_name + "\042;";
                                rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

                                vector<board_row> board_list;
                                while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                                {
                                    board_row tmp;

                                    tmp.INDEX = sqlite3_column_int(stmt, 0);
                                    tmp.NAME = std::string(strdup((char *)sqlite3_column_text(stmt, 1)));
                                    tmp.MODERATOR = std::string(strdup((char *)sqlite3_column_text(stmt, 2)));
                                    //tmp.DATE = std::string(strdup((char *)sqlite3_column_text(stmt, 3)));

                                    //cout << tmp.ID << " " << tmp.TITLE << " " << tmp.AUTHOR << " " << tmp.DATE << endl;
                                    board_list.push_back(tmp);
                                    //...
                                }
                                sqlite3_finalize(stmt);

                                string mes = "\n\tIndex\tName\t\tModerator\n";
                                char tmp[1024];
                                for (int j = 0; j < board_list.size(); j++)
                                {
                                    sprintf(tmp, "\t%-8d%-16.16s%-16.16s\n", board_list[j].INDEX, board_list[j].NAME.c_str(), board_list[j].MODERATOR.c_str());
                                    string t;
                                    t.assign(tmp);
                                    mes = mes + t;
                                    //mes = mes + "\t" + post_list[j].ID + "\t" + post_list[j].TITLE + "\t\t" + post_list[j].AUTHOR + "\t\t" + post_list[j].DATE + "\n";
                                }
                                int byte_send;
                                byte_send = send(i, mes.c_str(), mes.length(), 0);
                                //cout << byte_send << endl;
                                /////////////////////////////////////////////////
                                /*rc = sqlite3_exec(db, sql.c_str(), callback_sendback_message, (void *)data, &zErrMsg);
                                if (rc != SQLITE_OK)
                                {
                                    fprintf(stderr, "SQL error: %s", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                }
                                else
                                {
                                    send(i, sendback_message.c_str(), sendback_message.length(), 0);
                                    sendback_message = "";
                                }*/
                            }

                            //sql = "SELECT name FROM sqlite_master WHERE type ='table' AND name NOT LIKE 'sqlite_%';";
                        }
                        else if (command.compare("list-post") == 0)
                        {
                            string board_name;
                            ss >> board_name;
                            //cout << board_name << endl;
                            clientInfo_array[i].pos = board_name;
                            //sql = "SELECT ID,TITLE,AUTHOR,DATE FROM \042" + board_name + "\042;";
                            //cout << sql << endl;
                            //sendback_message = "\tID\tTitle\t\tAuthor\t\tDate\n";
                            //cout << sendback_message << endl;

                            sqlite3_stmt *stmt3;
                            sql = "SELECT COUNT(*) FROM  BOARD  WHERE NAME = \042" + board_name + "\042;";
                            rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt3, NULL);

                            int match_lines;

                            while ((rc = sqlite3_step(stmt3)) == SQLITE_ROW)
                            {
                                match_lines = sqlite3_column_int(stmt3, 0);
                                //cout << "match_lines: " << match_lines << endl;
                            }
                            sqlite3_finalize(stmt3);

                            if (match_lines == 0)
                            {
                                char mes[] = "Board does not exist.\n";
                                send(i, mes, sizeof(mes), 0);
                                continue;
                            }

                            if (ss.rdbuf()->in_avail() - 2 != 0)
                            {
                                string query;
                                ss >> query;
                                query.erase(std::remove(query.begin(), query.end(), '#'), query.end());

                                sql = "SELECT ID,TITLE,AUTHOR,strftime('%m/%d',DATE) FROM POST WHERE BOARD LIKE '" + board_name + "' AND TITLE LIKE '%" + query + "%';";
                            }
                            else
                            {
                                sql = "SELECT ID,TITLE,AUTHOR,strftime('%m/%d',DATE) FROM POST WHERE BOARD LIKE '" + board_name + "';";
                            }

                            //cout << sql << endl;

                            sqlite3_stmt *stmt;

                            rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

                            vector<post_row> post_list;
                            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                            {
                                post_row tmp;
                                //char title_tmp[] = strdup((char *)sqlite3_column_text(stmt, 1));
                                //char author_tmp[] = strdup((char *)sqlite3_column_text(stmt, 2));
                                //char  date_tmp[]=strdup((char *)sqlite3_column_text(stmt, 3));
                                tmp.ID = sqlite3_column_int(stmt, 0);
                                tmp.TITLE = std::string(strdup((char *)sqlite3_column_text(stmt, 1)));
                                tmp.AUTHOR = std::string(strdup((char *)sqlite3_column_text(stmt, 2)));
                                tmp.DATE = std::string(strdup((char *)sqlite3_column_text(stmt, 3)));
                                //tmp.DATE = std::to_string(sqlite3_column_double(stmt, 3));
                                //tmp.DATE = sqlite3_column_double(stmt, 3);
                                //cout << tmp.ID << " " << tmp.TITLE << " " << tmp.AUTHOR << " " << tmp.DATE << endl;
                                post_list.push_back(tmp);
                                //...
                                //cout << tmp.DATE << endl;
                            }

                            sqlite3_finalize(stmt);

                            string mes = "\n\tID\tTitle\t\tAuthor\t\tDate\n";
                            char tmp[1024];
                            for (int j = 0; j < post_list.size(); j++)
                            {
                                sprintf(tmp, "\t%-8d%-16.16s%-16.16s%-8.8s\n", post_list[j].ID, post_list[j].TITLE.c_str(), post_list[j].AUTHOR.c_str(), post_list[j].DATE.c_str());
                                string t;
                                t.assign(tmp);
                                mes = mes + t;
                                //mes = mes + "\t" + post_list[j].ID + "\t" + post_list[j].TITLE + "\t\t" + post_list[j].AUTHOR + "\t\t" + post_list[j].DATE + "\n";
                            }
                            //cout << "mes:\n"
                            //     << mes << endl;
                            int byte_send = send(i, mes.c_str(), mes.size(), 0);
                            //cout << "list-post byte send:" << byte_send << endl;
                            /*% list
                            rc = sqlite3_exec(db, sql.c_str(), callback_list_post, (void *)data, &zErrMsg);
                            if (rc != SQLITE_OK)
                            {
                                fprintf(stderr, "SQL error: %s", zErrMsg);
                                sqlite3_free(zErrMsg);
                            }
                            else
                            {
                                send(i, list_post.c_str(), list_post.length(), 0);
                                //cout << "list post:\n"
                                //    << list_post << endl;
                                //list_post.clear();
                                clientInfo_array[i].pos = board_name;
                            }
                            */
                        }
                        else if (command.compare("read") == 0)
                        {

                            string post_ID;
                            ss >> post_ID;

                            sqlite3_stmt *stmt3;
                            sql = "SELECT COUNT(*) FROM  POST  WHERE ID = \042" + post_ID + "\042;";
                            rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt3, NULL);

                            int match_lines;

                            while ((rc = sqlite3_step(stmt3)) == SQLITE_ROW)
                            {
                                match_lines = sqlite3_column_int(stmt3, 0);
                                //cout << "match_lines: " << match_lines << endl;
                            }

                            sqlite3_finalize(stmt3);

                            if (match_lines)
                            {
                                sqlite3_stmt *stmt;
                                sql = "SELECT AUTHOR,TITLE,date(`DATE`),CONTENT,COMMENT FROM POST WHERE ID==" + post_ID + ";";
                                rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
                                post_row tmp;
                                string comm;
                                while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                                {
                                    //author_name = strdup((char *)sqlite3_column_text(stmt, 0));
                                    //...

                                    tmp.AUTHOR = std::string(strdup((char *)sqlite3_column_text(stmt, 0)));
                                    tmp.TITLE = std::string(strdup((char *)sqlite3_column_text(stmt, 1)));
                                    //tmp.DATE = std::to_string(sqlite3_column_text(stmt, 2));
                                    //tmp.DATE = sqlite3_column_double(stmt, 2);
                                    tmp.DATE = std::string(strdup((char *)sqlite3_column_text(stmt, 2)));
                                    tmp.CONTENT = std::string(strdup((char *)sqlite3_column_text(stmt, 3)));
                                    if (sqlite3_column_type(stmt, 4) == SQLITE_NULL)
                                        tmp.COMMENT = "";
                                    else
                                        tmp.COMMENT = std::string(strdup((char *)sqlite3_column_text(stmt, 4)));

                                    //cout << "tmp.DATE:" << tmp.DATE << endl;
                                    //cout << "strftime:" << strftime('%m-%d', tmp.DATE.c_str()) << endl;
                                    //tmp.COMMENT = std::string(strdup((char *)sqlite3_column_text(stmt, 4)));
                                }
                                sqlite3_finalize(stmt);

                                string mes;
                                if (tmp.COMMENT == "")
                                {
                                    mes = "\tAuthor:\t" + tmp.AUTHOR + "\n\tTitle:\t" + tmp.TITLE + "\n\tDate:\t" + tmp.DATE + "\n\t--\n\t" + tmp.CONTENT + "\n\t--\n";
                                }
                                else
                                {
                                    mes = "\tAuthor:\t" + tmp.AUTHOR + "\n\tTitle:\t" + tmp.TITLE + "\n\tDate:\t" + tmp.DATE + "\n\t--\n\t" + tmp.CONTENT + "\n\t--\t" + tmp.COMMENT + "\n";
                                }

                                ReplaceStringInPlace(mes, "<br>", "\n\t");
                                int bytes_send;
                                bytes_send = send(i, mes.c_str(), mes.length(), 0);
                                //cout << bytes_send << endl;
                            }
                            else
                            {
                                char mes[] = "Post is not exist.\n";
                                send(i, mes, sizeof(mes), 0);
                            }

                            /////////////////////////////

                            //cout << "comm:" << comm << endl;
                            ////////////////////////////
                            /*sql = "SELECT AUTHOR,TITLE,DATE,CONTENT FROM \042" + clientInfo_array[i].pos + "\042 WHERE ID==" + post_ID + ";";

                                rc = sqlite3_exec(db, sql.c_str(), callback_read, (void *)data, &zErrMsg);
                                if (rc != SQLITE_OK)
                                {
                                    fprintf(stderr, "SQL error: %s", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                }
                                else
                                {
                                    int bytes_send;
                                    ReplaceStringInPlace(sendback_message, "<br>", "\n\t");
                                    bytes_send = send(i, sendback_message.c_str(), sendback_message.length(), 0);
                                    cout << bytes_send << endl;
                                }*/

                            //cout << "sendback:  " << endl
                            //    << sendback_message << endl;
                        }
                        else if (command.compare("delete-post") == 0)
                        {
                            string post_ID;
                            ss >> post_ID;
                            if (clientInfo_array[i].name == "anonymous")
                            {
                                char mes[] = "Please login first.\n";
                                send(i, mes, sizeof(mes), 0);
                            }
                            else
                            {
                                sqlite3_stmt *stmt;
                                sql = "SELECT COUNT(*) FROM POST WHERE ID = \042" + post_ID + "\042;";
                                rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

                                int match_lines;

                                while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                                {
                                    match_lines = sqlite3_column_int(stmt, 0);
                                    //cout << "match_lines: " << match_lines << endl;
                                }

                                sqlite3_finalize(stmt);

                                if (match_lines == 0)
                                {
                                    char mes[] = "Post is not exist.\n";
                                    send(i, mes, sizeof(mes), 0);
                                }
                                else
                                {
                                    sqlite3_stmt *stmt2;
                                    sql = "SELECT AUTHOR FROM POST WHERE ID = \042" + post_ID + "\042;";
                                    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt2, NULL);

                                    //cout << sql << endl;
                                    const char *author_name;
                                    while ((rc = sqlite3_step(stmt2)) == SQLITE_ROW)
                                    {
                                        author_name = strdup((char *)sqlite3_column_text(stmt2, 0));
                                        //...
                                    }
                                    sqlite3_finalize(stmt2);
                                    //cout << author_name << endl;

                                    if (strcmp(clientInfo_array[i].name.c_str(), author_name) == 0)
                                    {
                                        // delete
                                        sql = "DELETE FROM POST WHERE ID = \042" + post_ID + "\042 AND AUTHOR = \042" + clientInfo_array[i].name + "\042;";

                                        rc = sqlite3_exec(db, sql.c_str(), callback, (void *)data, &zErrMsg);
                                        if (rc != SQLITE_OK)
                                        {
                                            fprintf(stderr, "SQL error: %s", zErrMsg);
                                            sqlite3_free(zErrMsg);
                                        }
                                        else
                                        {
                                            int cnt;
                                            cnt = sqlite3_changes(db);

                                            if (cnt == 1)
                                            {
                                                char mes[] = "Delete Successfully.\n";
                                                send(i, mes, sizeof(mes), 0);
                                            }
                                            else
                                            {
                                                char mes[] = "Delete falies.\n";
                                                send(i, mes, sizeof(mes), 0);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        char mes[] = "Not the post owner.\n";
                                        send(i, mes, sizeof(mes), 0);
                                    }
                                }
                            }
                        }
                        else if (command.compare("update-post") == 0)
                        {
                            if (clientInfo_array[i].name == "anonymous")
                            {
                                char mes[] = "Please login first.\n";
                                send(i, mes, sizeof(mes), 0);
                            }
                            else
                            {
                                string post_ID, __command;
                                ss >> post_ID;
                                ss >> __command;

                                if (__command.compare("--title") == 0)
                                {
                                    sqlite3_stmt *stmt;
                                    sql = "SELECT COUNT(*) FROM POST WHERE ID = \042" + post_ID + "\042;";
                                    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

                                    int match_lines;

                                    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                                    {
                                        match_lines = sqlite3_column_int(stmt, 0);
                                        //cout << "match_lines: " << match_lines << endl;
                                    }

                                    sqlite3_finalize(stmt);

                                    if (match_lines == 0)
                                    {
                                        char mes[] = "Post is not exist.\n";
                                        send(i, mes, sizeof(mes), 0);
                                    }
                                    else
                                    {
                                        sqlite3_stmt *stmt;
                                        sql = "SELECT AUTHOR FROM POST WHERE ID = \042" + post_ID + "\042;";
                                        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
                                        const char *author_name;
                                        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                                        {
                                            author_name = strdup((char *)sqlite3_column_text(stmt, 0));
                                            //...
                                        }
                                        sqlite3_finalize(stmt);
                                        //cout << author_name << endl;

                                        if (strcmp(clientInfo_array[i].name.c_str(), author_name) == 0)
                                        {
                                            string line(buf);
                                            int title_pos; //16
                                            title_pos = line.find("--title");
                                            //cout << "title_pos " << title_pos << endl;

                                            int title_length;                              //33
                                            title_length = line.length() - title_pos - 10; //30 - 14 - 9 = 7
                                            //cout << "line length" << line.length() << endl;
                                            //cout << "title_length" << title_length << endl;

                                            string title;
                                            title = line.substr(title_pos + 8, title_length); //22,7

                                            //cout << "new title:" << title << endl;
                                            //cout << "new title length:" << title.length() << endl;
                                            //string update_sql = "UPDATE 'NCTU99' SET TITLE = 'fixed' WHERE ID = \0422\042;";
                                            string update_sql = "UPDATE POST SET TITLE = '" + title + "' WHERE ID = " + post_ID + ";";

                                            //update_sql = update_sql + title + "'WHERE ID = " + post_ID + ";";

                                            //cout << "update sql2:\n"
                                            //     << update_sql << endl;

                                            rc = sqlite3_exec(db, update_sql.c_str(), callback, (void *)data, &zErrMsg);
                                            if (rc != SQLITE_OK)
                                            {

                                                fprintf(stderr, "SQL error: %s", zErrMsg);
                                                sqlite3_free(zErrMsg);
                                            }

                                            int update_lines = sqlite3_changes(db);
                                            //cout << "update_lines:" << update_lines << endl;
                                            if (update_lines == 1)
                                            {
                                                char mes[] = "Update successfully.\n";
                                                send(i, mes, sizeof(mes), 0);
                                            }
                                            else
                                            {
                                                char mes[] = "Update failes.\n";
                                                send(i, mes, sizeof(mes), 0);
                                            }
                                        }
                                        else
                                        {
                                            char mes[] = "Not the post owner.\n";
                                            send(i, mes, sizeof(mes), 0);
                                        }
                                    }
                                }
                                else if (__command.compare("--content") == 0)
                                {

                                    sqlite3_stmt *stmt;
                                    sql = "SELECT COUNT(*) FROM POST WHERE ID = \042" + post_ID + "\042;";
                                    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

                                    int match_lines;

                                    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                                    {
                                        match_lines = sqlite3_column_int(stmt, 0);
                                        //cout << "match_lines: " << match_lines << endl;
                                    }

                                    sqlite3_finalize(stmt);

                                    if (match_lines == 0)
                                    {
                                        char mes[] = "Post is not exist.\n";
                                        send(i, mes, sizeof(mes), 0);
                                    }
                                    else
                                    {
                                        sqlite3_stmt *stmt;
                                        sql = "SELECT AUTHOR FROM POST WHERE ID = \042" + post_ID + "\042;";
                                        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
                                        const char *author_name;
                                        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                                        {
                                            author_name = strdup((char *)sqlite3_column_text(stmt, 0));
                                            //...
                                        }
                                        sqlite3_finalize(stmt);
                                        //cout << author_name << endl;

                                        if (strcmp(clientInfo_array[i].name.c_str(), author_name) == 0)
                                        {
                                            string line(buf);
                                            int content_pos;
                                            content_pos = line.find("--content");

                                            int content_length;
                                            content_length = line.length() - 2 - content_pos - 10;
                                            string content;
                                            content = line.substr(content_pos + 10, content_length);

                                            //cout << "new content:" << content << endl;
                                            //cout << "new content length: " << content.length() << endl;
                                            sql = "UPDATE POST SET CONTENT = \042" + content +
                                                  "\042 WHERE ID = \042" + post_ID + "\042;";

                                            //cout << "sql :" << sql << endl;

                                            rc = sqlite3_exec(db, sql.c_str(), callback, (void *)data, &zErrMsg);
                                            int update_lines = sqlite3_changes(db);
                                            //cout << "update_lines:" << update_lines << endl;
                                            if (update_lines == 1)
                                            {
                                                char mes[] = "Update successfully.\n";
                                                send(i, mes, sizeof(mes), 0);
                                            }
                                            else
                                            {
                                                char mes[] = "Update failes.\n";
                                                send(i, mes, sizeof(mes), 0);
                                            }
                                        }
                                        else
                                        {
                                            char mes[] = "Not the post owner.\n";
                                            send(i, mes, sizeof(mes), 0);
                                        }
                                    }
                                }
                                else
                                {
                                    char mes[] = "Usage: update-post <post-id> --title/content <new>\n";
                                    send(i, mes, sizeof(mes), 0);
                                }
                            }
                        }
                        else if (command.compare("comment") == 0)
                        {
                            if (clientInfo_array[i].name == "anonymous")
                            {
                                char mes[] = "Please login first.\n";
                                send(i, mes, sizeof(mes), 0);
                            }
                            else
                            {
                                string comment_ID;
                                ss >> comment_ID;
                                sqlite3_stmt *stmt;
                                sql = "SELECT COUNT(*) FROM POST WHERE ID = \042" + comment_ID + "\042;";
                                rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

                                int match_lines;

                                while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                                {
                                    match_lines = sqlite3_column_int(stmt, 0);
                                    //cout << "match_lines: " << match_lines << endl;
                                }

                                sqlite3_finalize(stmt);

                                if (match_lines == 0)
                                {
                                    char mes[] = "Post is not exist.\n";
                                    send(i, mes, sizeof(mes), 0);
                                }
                                else
                                { /////////////
                                    sql = "SELECT COMMENT FROM POST WHERE ID = \042" + comment_ID + "\042;";
                                    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
                                    if (rc != SQLITE_OK)
                                    {
                                        printf("error: %s\n", sqlite3_errmsg(db));
                                        // or throw an exception
                                        //return -1;
                                    }
                                    string comment;
                                    //cout << "string comment" << comment << endl;
                                    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
                                    {
                                        if (sqlite3_column_type(stmt, 0) == SQLITE_NULL)
                                        {
                                            comment = "";
                                            //cout << "first comment" << endl;
                                        }
                                        else
                                        {
                                            comment = std::string(strdup((char *)sqlite3_column_text(stmt, 0)));
                                        }
                                        //cout << "Null" << endl;

                                        //
                                        //tmp.TITLE = std::string(strdup((char *)sqlite3_column_text(stmt, 1)));
                                    }
                                    //cout << "string comment" << comment << endl;

                                    //////////////     % comment 1 test     12~15
                                    string line(buf);
                                    int comment_pos = line.find(comment_ID);
                                    int comment_length = line.length() - 2 - comment_pos - 1 - comment_ID.length();
                                    /*
                                    cout << "line.length();" << line.length() << endl;
                                    cout << "comment_pos:" << comment_pos << endl;
                                    cout << "comment_ID.length():" << comment_ID.length() << endl;
                                    cout << "comment_length:" << comment_length << endl;
*/
                                    string new_comment = "";
                                    new_comment = line.substr(comment_pos + comment_ID.length() + 1, comment_length);
                                    /*cout << "new_comment:" << endl
                                         << new_comment << endl;
                                    cout << "new_comment.length" << new_comment.length() << endl;
*/
                                    comment = comment + "\n\t" + clientInfo_array[i].name + ": " + new_comment;
                                    //cout << "final comment:" << endl
                                    //    << comment << endl;

                                    sql = "UPDATE POST SET COMMENT = '" + comment + "' WHERE ID = '" + comment_ID + "';";

                                    // cout << "comment update sql:" << endl;
                                    //cout << sql << endl;

                                    rc = sqlite3_exec(db, sql.c_str(), callback, (void *)data, &zErrMsg);
                                    if (rc != SQLITE_OK)
                                    {
                                        fprintf(stderr, "SQL error: %s", zErrMsg);
                                        sqlite3_free(zErrMsg);
                                    }

                                    int update_lines = sqlite3_changes(db);
                                    if (update_lines == 1)
                                    {
                                        char mes[] = "Comment successfully.\n";
                                        send(i, mes, sizeof(mes), 0);
                                    }
                                    else
                                    {
                                        char mes[] = "Comment failes.\n";
                                        send(i, mes, sizeof(mes), 0);
                                    }
                                }
                            }
                        }
                        else
                        {
                            char mes[] = "Please input a correct command!\n";
                            send(i, mes, sizeof(mes), 0);
                        }

                        memset(buf, 0, sizeof(buf));
                    }
                } // END handle data from client
            }     // END got new incoming connection
        }         // END looping through file descriptors
    }             // END for( ; ; )--and you thought it would never end!

    return 0;
}
