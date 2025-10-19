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

int main(int argc, char **argv) {
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

    const char* get_ping = "get_ping";

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

            double pings[5][5] = {{0.0}};

            for (int i = 1; i <= 5; i++) {
                pings[i - 1][i - 1] = -1;
                for (int j = 1; j <= 5; j++) {
                    if (i == j) continue;
                    if (i == this) {
                        char target_node[16];
                        sprintf(target_node, "node%d", j);
                        
                        char ping_cmd[128];
                        sprintf(ping_cmd, "ping -c 3 %s | grep 'rtt min/avg/max/mdev' | awk -F'/' '{print $5}'", target_node);
                        FILE* fp = popen(ping_cmd, "r");
                        if (fp == NULL) {
                            perror("popen");
                            return 1;
                        }

                        double rtt_avg = 0.0;
                        if (fscanf(fp, "%lf", &rtt_avg) == 1) {
                            pings[i - 1][j - 1] = rtt_avg;
                            printf("RTT between node%d and %s: %.3f ms\n", this, target_node, rtt_avg);
                        } else {
                            printf("Failed to parse ping result for %s\n", target_node);
                        }
                        pclose(fp);
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

                        char ping_cmd[16];
                        sprintf(ping_cmd, "%s %d", get_ping, j);
                        int n = strlen(ping_cmd);
                        if ((n = sendto(sock, ping_cmd, n, 0,
                                        res->ai_addr, res->ai_addrlen)) != n) {
                                            perror("sendto");
                                            return -11;
                        }

                        sleep(1);
                        addrLen = sizeof(clientAddr);
                        n = recvfrom(sock, buf, BUF_LEN, 0,
                                    (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
                        if (n < 0) {
                            perror("recvfrom");
                        }
                        buf[n] = '\0';

                        double rtt_avg = 0.0;
                        sscanf(buf, "%lf", &rtt_avg);
                        
                        printf("RTT between %s and node%d: %.3f ms\n", target_node, j, rtt_avg);
                        pings[i - 1][j - 1] = rtt_avg;
                        close(sock);
                    }
                }
                printf("\n");
            }
            printf("\n--- Ping Latency Matrix (ms) ---\n\n");
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
                        printf(" %10.3f ", pings[i][j]);
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

                if (strlen(buf) >= strlen(get_ping) && strncmp(buf, get_ping, strlen(get_ping)) == 0) {
                    int target;
                    sscanf(buf, "get_ping %d", &target);
                    char target_node[16];
                    sprintf(target_node, "node%d", target);

                    char ping_cmd[128];
                    sprintf(ping_cmd, "ping -c 3 %s | grep 'rtt min/avg/max/mdev' | awk -F'/' '{print $5}'", target_node);
                    FILE* fp = popen(ping_cmd, "r");
                    if (fp == NULL) {
                        perror("popen");
                        return 1;
                    }

                    double rtt_avg = 0.0;
                    if (fscanf(fp, "%lf", &rtt_avg) == 1) {
                        printf("RTT between node%d and %s: %.3f ms\n", this, target_node, rtt_avg);
                    } else {
                        printf("Failed to parse ping result for %s\n", target_node);
                    }
                    pclose(fp);

                    sleep(2);
                    char c_rtt[16];
                    sprintf(c_rtt, "%lf", rtt_avg);
                    n = strlen(c_rtt);
                    if ((n = sendto(sock, c_rtt, n, 0,
                                    (struct sockaddr *)&clientAddr, addrLen)) != n) {
                                        perror("sendto");
                                        return -11;
                    }
                } else if (strncmp(buf, "quit", 4) == 0)
                    break;
            }

            close(sock);
            
            break;
    }

    return 0;
}