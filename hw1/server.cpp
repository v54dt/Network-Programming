
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

using namespace std;

struct clientInfo
{
  int fd;
  sockaddr_in addr;
  char ip[20];
  unsigned short port;
  string name;
  clientInfo() : name("annoymous"), fd(-1) {}
};

clientInfo clientInfo_array[30];
sqlite3 *db;
char *zErrMsg = 0;
int rc;
string sql;

string password;

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
  // printf("");
  password.assign(argv[1]);

  return 0;
}

void create_database()
{
  rc = sqlite3_open("auth.db", &db);
  if (rc)
  {
    // fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  }
  else
  {
    // fprintf(stderr, "Opened database successfully\n");

    sql =
        "CREATE TABLE ACCOUNTS("
        "ID CHAR(50) PRIMARY KEY NOT NULL,"
        "PASSWORD CHAR(50) NOT NULL);";

    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
      // fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
    else
    {
      // fprintf(stdout, "Table created successfully\n");
    }
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
            printf("New Connection.\n");
            char mes[] =
                "******************************\n"
                "**Welcome to the BBS server.**\n"
                "******************************\n";
            // cout << mes << endl;
            send(newfd, mes, sizeof(mes), 0);

            /*printf(
                "New Connection from %s on "
                "socket %d\n",
                inet_ntop(remoteaddr.ss_family,
                          get_in_addr((struct sockaddr *)&remoteaddr), remoteIP,
                          INET6_ADDRSTRLEN),
                newfd);*/
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
            string command_prompt;
            ss >> command_prompt;

            // cout << "command_prompt is:" << command_prompt << endl;
            if (command_prompt.compare(0, 1, "%") != 0)
            {
              char mes[] = "Please input % first\n";
              send(i, mes, sizeof(mes), 0);
              continue;
            }

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
              // cout << "k:" << k << endl;
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
                rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
                if (rc != SQLITE_OK)
                {

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
              if (clientInfo_array[i].name != "annoymous")
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
                  rc = sqlite3_exec(db, sql.c_str(), callback, (void *)data,
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
              if (clientInfo_array[i].name == "annoymous")
              {
                char mes[] = "Please login first.\n";
                send(i, mes, sizeof(mes), 0);
              }
              else
              {
                string mes = "Bye, " + clientInfo_array[i].name + ".\n";
                send(i, mes.c_str(), mes.length(), 0);

                clientInfo_array[i].name = "annoymous";
              }
            }
            else if (command.compare("whoami") == 0)
            {
              // cout << i << endl;
              if (clientInfo_array[i].name != "annoymous")
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
              clientInfo_array[i].name = "annoymous";
              close(i);
              FD_CLR(i, &master);
            }
            else if (command.compare("check") == 0)
            {
              sql = "SELECT * from ACCOUNTS ;";

              /* Execute SQL statement */
              rc = sqlite3_exec(db, sql.c_str(), callback, (void *)data,
                                &zErrMsg);
              if (rc != SQLITE_OK)
              {
                fprintf(stderr, "SQL error: %s", zErrMsg);
                sqlite3_free(zErrMsg);
              }
              else
              {
                // fprintf(stdout, "Operation done successfully");
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
      }   // END got new incoming connection
    }     // END looping through file descriptors
  }       // END for( ; ; )--and you thought it would never end!

  return 0;
}
