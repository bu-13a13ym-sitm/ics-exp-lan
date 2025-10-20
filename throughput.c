/* -*- mode: c; coding:utf-8; ##: nil; -*-                          */
/*                                                                  */
/*  FILENAME     :  lab_udp_ecli.c                                  */
/*  DESCRIPTION  :  UDP echo clientの練習                           */
/*                                                                  */
/*  DATE         :  Sep. 01, 2020                                   */
/*                                                                  */

#include <stdbool.h>
#include <signal.h>
#include "icslab2_net.h"

#define MAX_EVENTS 16

#define SIZE 5
#define MAX_PATHS 100

double edge[SIZE][SIZE];

char ip_addresses[5][5][128] = {
    {"127.0.0.1", "172.20.0.20", "172.21.0.30", "172.22.0.40", "172.23.0.50"},
    {"172.20.0.10", "127.0.0.1", "172.24.0.30", "172.25.0.40", "172.26.0.50"},
    {"172.21.0.10", "172.24.0.20", "127.0.0.1", "172.27.0.40", "172.28.0.50"},
    {"172.22.0.10", "172.25.0.20", "172.27.0.30", "127.0.0.1", "172.29.0.50"},
    {"172.23.0.10", "172.26.0.20", "172.28.0.30", "172.29.0.40", "127.0.0.1"}
};

typedef struct {
    int nodes[SIZE + 1]; // 経路（ノード番号の配列）
    int length;          // 経路の長さ
    double weight;        // 経路の重み
} PathInfo;

PathInfo paths[MAX_PATHS];
int path_count = 0;

// 経路を再帰的に列挙
void enumerate_paths(int current, int goal, bool visited[SIZE], int route[SIZE + 1], int depth, float weight) {
    visited[current] = true;
    route[depth] = current;

    if (current == goal) {
        // 経路を保存
        paths[path_count].length = depth + 1;
        paths[path_count].weight = weight;
        for (int i = 0; i <= depth; i++) {
            paths[path_count].nodes[i] = route[i];
        }
        path_count++;
    } else {
        for (int next = 0; next < SIZE; next++) {
            if (!visited[next] && edge[current][next] > 0.0) {
                enumerate_paths(next, goal, visited, route, depth + 1, weight + edge[current][next]);
            }
        }
    }
    visited[current] = false;
}

// 重みで昇順ソート
int compare_paths(const void *a, const void *b) {
    float wa = ((PathInfo*)a)->weight;
    float wb = ((PathInfo*)b)->weight;
    if (wa < wb) return -1; // 小さい順に並び替え。大きい順なら逆
    if (wa > wb) return 1;
    return 0;
}



int main(int argc, char **argv)
{
    double throughputs[5][5] = {{0.0}};
    char *port_num_str = UDP_SERVER_PORT_STR;        /* ポート番号 */

    int sock;                       /* ソケットディスクリプタ */
    struct sockaddr_in serverAddr;  /* サーバ＝相手用のアドレス構造体 */
    struct sockaddr_in clientAddr;  /* クライアント＝自分用のアドレス構造体 */
    int addrLen;                    /* serverAddrのサイズ */
    char buf[BUF_LEN];              /* 受信バッファ */
    int n;                          /* 読み込み／受信バイト数 */

    unsigned int this;
    char mode = 's';

    int err;

    const char* iperf3_s = "iperf3_s";
    const char* server_started = "server_started";
    const char* kill_iperf3 = "kill_iperf3";
    const char* server_stopped = "server_stopped";

    /* コマンドライン引数の処理 */
    if(argc == 2 && strncmp(argv[1], "-h", 2) == 0) {
        printf("Usage: %s [num_this_nord] [mode]\n", argv[0]);
        return 0;
    }
    if(argc > 1)
        this = (unsigned int)atoi(argv[1]);
    if (argc > 2)
        mode = argv[2][0];

    switch (mode) {
        case ('c'):
            printf("started as client\n\n");


            for (int i = 1; i <= 5; i++) {
                throughputs[i - 1][i - 1] = -1;
                for (int j = 1; j <= 5; j++) {
                    if (i == j) continue;
                    if (i == this) {
                        char target_node[16];
                        sprintf(target_node, "node%d", j);
                        /* ホスト名からIPアドレスを検索する */
                        struct addrinfo hints, *res;
                        memset(&hints, 0, sizeof(hints));
                        hints.ai_family = AF_INET;          /* IPv4で通信 */
                        hints.ai_socktype = SOCK_DGRAM;     /* UDP */

                        err = getaddrinfo(target_node, port_num_str, &hints, &res);
                        if(err != 0) {
                            perror("getaddrinfo");
                            return 1;
                        }
                        
                        sock = socket(res->ai_family, res->ai_socktype, 0);
                        if(sock < 0) {
                            perror("socket");
                            return  1;
                        }

                        int n = strlen(iperf3_s);
                        if ((n = sendto(sock, iperf3_s, n, 0,
                                    res->ai_addr, res->ai_addrlen)) != n) {
                                        perror("sendto");
                                        return -11;
                        }

                        addrLen = sizeof(clientAddr);
                        n = recvfrom(sock, buf, BUF_LEN, 0,
                                    (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
                        if (n < 0) {
                            perror("recvfrom");
                        }
                        buf[n] = '\0';

                        if (n >= strlen(server_started) && strncmp(buf, server_started, strlen(server_started)) == 0)
                            printf("%s iperf3 server started\n", target_node);
                        else
                            printf("%s iperf3 server hasn't started\n", target_node);
                        
                        char iperf3_c[128];
                        double throughput = 0.0;
                        sprintf(iperf3_c, "iperf3 -c node%d -u -4 -b 1G -t 3 --json | grep bits_per_second", j);
                        FILE* fp = popen(iperf3_c, "r");
                        if (fp == NULL) {
                            perror("popen");
                            return 1;
                        }

                        while (fgets(buf, sizeof(buf), fp) != NULL) {
                            sscanf(buf, " \"bits_per_second\": %lf", &throughput);
                        }
                        pclose(fp);
                        printf("throughput between node%d and %s: %lf\n", this, target_node, throughput);
                        throughputs[i - 1][j - 1] = throughput;

                        n = strlen(kill_iperf3);
                        if ((n = sendto(sock, kill_iperf3, n, 0,
                                    res->ai_addr, res->ai_addrlen)) != n) {
                                        perror("sendto");
                                        return -11;
                        }

                        addrLen = sizeof(clientAddr);
                        n = recvfrom(sock, buf, BUF_LEN, 0,
                                    (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
                        if (n < 0) {
                            perror("recvfrom");
                        }
                        buf[n] = '\0';

                        if (n >= strlen(server_stopped) && strncmp(buf, server_stopped, strlen(server_stopped)) == 0)
                            printf("%s iperf3 server killed\n", target_node);
                        else
                            printf("%s iperf3 server hasn't been stopped\n", target_node);
                        close(sock);
                    }
                    else {
                        char target_node[16];
                        sprintf(target_node, "node%d", i);
                        /* ホスト名からIPアドレスを検索する */
                        struct addrinfo hints, *res;
                        memset(&hints, 0, sizeof(hints));
                        hints.ai_family = AF_INET;          /* IPv4で通信 */
                        hints.ai_socktype = SOCK_DGRAM;     /* UDP */

                        err = getaddrinfo(target_node, port_num_str, &hints, &res);
                        if(err != 0) {
                            perror("getaddrinfo");
                            return 1;
                        }

                        
                        sock = socket(res->ai_family, res->ai_socktype, 0);
                        if(sock < 0) {
                            perror("socket");
                            return  1;
                        }

                        char iperf3_c[16];
                        sprintf(iperf3_c, "iperf3_c %d", j);
                        int n = strlen(iperf3_c);
                        if ((n = sendto(sock, iperf3_c, n, 0,
                                        res->ai_addr, res->ai_addrlen)) != n) {
                                            perror("sendto");
                                            return -11;
                        }

                        if (j == this) {
                            int tmpsock;
                            /* STEP 1: UDPソケットをオープンする */
                            if((tmpsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                                perror("socket");
                                return  1;
                            }

                            memset(&serverAddr, 0, sizeof(serverAddr));     /* ゼロクリア */
                            serverAddr.sin_family = AF_INET;                /* Internetプロトコル */

                            /* STEP 2 xxx: 待ち受けるポート番号を 10000 (= UDP_SERVER_PORT)に設定 */
                            serverAddr.sin_port =  htons(UDP_SERVER_PORT);
                            
                            serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* どのIPアドレス宛でも */

                            if(( bind(tmpsock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) ) < 0) {
                                perror("bind");
                                return  1;
                            }

                            struct sockaddr_in sourceAddr;
                            addrLen = sizeof(sourceAddr);
                            n = recvfrom(tmpsock, buf, BUF_LEN, 0,
                                        (struct sockaddr *)&sourceAddr, (socklen_t *)&addrLen);
                            if(n < 0) {
                                perror("recvfrom");
                                break;
                            }
                            buf[n] = '\0';

                            char* iperf3_c = "iperf3_c";

                            if (strlen(buf) >= strlen(iperf3_s) && strncmp(buf, iperf3_s, strlen(iperf3_s)) == 0) {
                                pid_t pid = fork();
                                if (pid == 0) {
                                    execlp("iperf3", "iperf3", "-s", NULL);
                                    perror("execlp failed");
                                    exit(1);
                                } else if (pid > 0) {
                                    for ( ; ; ) {
                                        if (sendto(tmpsock, server_started, strlen(server_started), 0,
                                                    (struct sockaddr *)&sourceAddr, addrLen) != strlen(server_started)) {
                                                        perror("sendto");
                                                        exit(1);
                                        }
                                        n = recvfrom(tmpsock, buf, BUF_LEN, 0,
                                                    (struct sockaddr *)&sourceAddr, (socklen_t *)&addrLen);
                                        if(n < 0) {
                                            perror("recvfrom");
                                            break;
                                        }
                                        buf[n] = '\0';

                                        if (strlen(buf) >= strlen(kill_iperf3) && strncmp(buf, kill_iperf3, strlen(kill_iperf3)) == 0) {
                                            kill(pid, SIGTERM);
                                            sleep(1);
                                            kill(pid, SIGTERM);

                                            if (sendto(tmpsock, server_stopped, strlen(server_stopped), 0,
                                                        (struct sockaddr *)&sourceAddr, addrLen) != strlen(server_stopped)) {
                                                perror("sendto");
                                                continue;
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                            close(tmpsock);
                        }

                        addrLen = sizeof(clientAddr);
                        n = recvfrom(sock, buf, BUF_LEN, 0,
                                    (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
                        if (n < 0) {
                            perror("recvfrom");
                        }
                        buf[n] = '\0';

                        double throughput = 0.0;
                        sscanf(buf, "%lf", &throughput);
                        
                        printf("throughput between %s and node%d: %lf\n", target_node, j, throughput);
                        throughputs[i - 1][j - 1] = throughput;
                        close(sock);
                    }
                }
                printf("\n");
            }
            printf("\n--- Throughput Measurement Matrix (bps) ---\n\n");
            printf("      ");
            for (int i = 1; i <= 5; i++) {
                printf("    node%-4d", i);
            }
            printf("\n");
            printf("      ------------------------------------------------------------\n");

            for (int i = 0; i < 5; i++) {
                printf("node%d |", i + 1);
                for (int j = 0; j < 5; j++) {
                    if (i == j) {
                        printf("      -     ");
                    } else {
                        printf(" %10.2f ", throughputs[i][j]);
                    }
                }
                printf("\n");
            }
            break;
        case ('s'):
        default:
            printf("started as server\n\n");

            /* STEP 1: UDPソケットをオープンする */
            if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("socket");
                return  1;
            }

            /* STEP 2: クライアントからの要求を受け付けるIPアドレスとポートを設定する */
            memset(&serverAddr, 0, sizeof(serverAddr));     /* ゼロクリア */
            serverAddr.sin_family = AF_INET;                /* Internetプロトコル */

            /* STEP 2 xxx: 待ち受けるポート番号を 10000 (= UDP_SERVER_PORT)に設定 */
            serverAddr.sin_port =  htons(UDP_SERVER_PORT);
            
            serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* どのIPアドレス宛でも */

            /* STEP 3:ソケットとアドレスをbindする */
            if(( bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr))/* ADD HERE */ ) < 0) {
                perror("bind");
                return  1;
            }

            for( ; ; ) {                            /* 無限ループ */
                /* STEP 4: クライアントからのデータグラムを受けとる */
                addrLen = sizeof(clientAddr);
                n = recvfrom(sock, buf, BUF_LEN, 0,
                            (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
                if(n < 0) {
                    perror("recvfrom");
                    break;
                }
                buf[n] = '\0';

                char* iperf3_c = "iperf3_c";

                if (strlen(buf) >= strlen(iperf3_s) && strncmp(buf, iperf3_s, strlen(iperf3_s)) == 0) {
                    pid_t pid = fork();
                    if (pid == 0) {
                        execlp("iperf3", "iperf3", "-s", NULL);
                        perror("execlp failed");
                        exit(1);
                    } else if (pid > 0) {
                        for ( ; ; ) {
                            sleep(1);
                            if (sendto(sock, server_started, strlen(server_started), 0,
                                        (struct sockaddr *)&clientAddr, addrLen) != strlen(server_started)) {
                                            perror("sendto");
                                            exit(1);
                            }
                            n = recvfrom(sock, buf, BUF_LEN, 0,
                                        (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
                            if(n < 0) {
                                perror("recvfrom");
                                break;
                            }
                            buf[n] = '\0';

                            if (strlen(buf) >= strlen(kill_iperf3) && strncmp(buf, kill_iperf3, strlen(kill_iperf3)) == 0) {
                                kill(pid, SIGTERM);
                                sleep(1);
                                kill(pid, SIGTERM);

                                sleep(1);
                                if (sendto(sock, server_stopped, strlen(server_stopped), 0,
                                            (struct sockaddr *)&clientAddr, addrLen) != strlen(server_stopped)) {
                                    perror("sendto");
                                    continue;
                                }
                                break;
                            }
                        }
                    }
                } else if (strlen(buf) >= strlen(iperf3_c) && strncmp(buf, iperf3_c, strlen(iperf3_c)) == 0) {
                    struct sockaddr_in original_clientAddr;
                    socklen_t original_addrLen = addrLen;
                    memcpy(&original_clientAddr, &clientAddr, sizeof(clientAddr));

                    int target;
                    sscanf(buf, "iperf3_c %d", &target);
                    char target_node[16];
                    sprintf(target_node, "node%d", target);
                    /* ホスト名からIPアドレスを検索する */
                    struct addrinfo hints, *res;
                    memset(&hints, 0, sizeof(hints));
                    hints.ai_family = AF_INET;          /* IPv4で通信 */
                    hints.ai_socktype = SOCK_DGRAM;     /* UDP */

                    err = getaddrinfo(target_node, port_num_str, &hints, &res);
                    if(err != 0) {
                        perror("getaddrinfo");
                        return 1;
                    }
                    
                    int csock = socket(res->ai_family, res->ai_socktype, 0);
                    if(csock < 0) {
                        perror("socket");
                        return  1;
                    }

                    sleep(1);
                    n = strlen(iperf3_s);
                    if ((n = sendto(csock, iperf3_s, n, 0,
                                res->ai_addr, res->ai_addrlen)) != n) {
                                    perror("sendto");
                                    return -11;
                    }

                    addrLen = sizeof(clientAddr);
                    n = recvfrom(csock, buf, BUF_LEN, 0,
                                (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
                    if (n < 0) {
                        perror("recvfrom");
                    }
                    buf[n] = '\0';

                    printf("%s iperf3 server started\n", target_node);
                    
                    char iperf3_c[128];
                    double throughput = 0.0;
                    sprintf(iperf3_c, "iperf3 -c node%d -u -4 -b 1G -t 3 --json | grep bits_per_second", target);
                    FILE* fp = popen(iperf3_c, "r");
                    if (fp == NULL) {
                        perror("popen");
                        return 1;
                    }

                    while (fgets(buf, sizeof(buf), fp) != NULL) {
                        sscanf(buf, " \"bits_per_second\": %lf", &throughput);
                    }
                    pclose(fp);
                    printf("throughput between node%d and %s: %lf\n", this, target_node, throughput);

                    n = strlen(kill_iperf3);
                    if ((n = sendto(csock, kill_iperf3, n, 0,
                                res->ai_addr, res->ai_addrlen)) != n) {
                                    perror("sendto");
                                    return -11;
                    }

                    addrLen = sizeof(clientAddr);
                    n = recvfrom(csock, buf, BUF_LEN, 0,
                                (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
                    if (n < 0) {
                        perror("recvfrom");
                    }
                    buf[n] = '\0';

                    if (n >= strlen(server_stopped) && strncmp(buf, server_stopped, strlen(server_stopped)) == 0)
                        printf("%s iperf3 server killed\n", target_node);
                    else
                        printf("%s iperf3 server hasn't been stopped\n", target_node);
                    close(csock);

                    char c_throughput[16];
                    sprintf(c_throughput, "%lf", throughput);
                    n = strlen(c_throughput);
                    if ((n = sendto(sock, c_throughput, n, 0,
                                    (struct sockaddr *)&original_clientAddr, original_addrLen)) != n) {
                                        perror("sendto");
                                        return -11;
                    }
                } else if (strncmp(buf, "quit", 4) == 0)
                    break;
            }

            close(sock);
            
            break;
    }
    
    int start = 2; // ノード3
    int goal = 0;  // ノード1
    bool visited[SIZE] = {false};
    int route[SIZE + 1];

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (i != j) {
                edge[i][j] = 1 / throughputs[i][j] * 1000*1000;
            }
        }
    }

    enumerate_paths(start, goal, visited, route, 0, 0.0);

    // 16経路のみを対象にする
    if (path_count > 16) path_count = 16;

    qsort(paths, path_count, sizeof(PathInfo), compare_paths);

    printf("重みが小さい順にすべての経路を出力します（ノード番号は1始まり）:\n");
    for (int i = 0; i < path_count; i++) {
        printf("重み: %.5f, ルート: ", paths[i].weight);
        for (int j = 0; j < paths[i].length; j++) {
            printf("%d", paths[i].nodes[j] + 1);
            if (j < paths[i].length - 1) printf("->");
        }
        printf("\n");
    }
    // paths[i]に各経路の情報が格納されている
    // paths[i].nodesに経路（ノードの番号の配列）、paths[i].lengthに経路の長さ、paths[i].weightに経路の重みが入っている


    return 0;
}