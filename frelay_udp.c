#include "icslab2_net.h"

int
main(int argc, char** argv)
{
    int     from_sock;              /* 受信用ソケットディスクリプタ */
    int     to_sock;                /* 転送用ソケットディスクリプタ */
    struct sockaddr_in  serverAddr; /* サーバ＝自分用アドレス構造体 */
    struct sockaddr_in  clientAddr; /* クライアント＝送信元アドレス構造体 */
    socklen_t addrLen;              /* clientAddrのサイズ */
    char    buf[BUF_LEN];           /* 受信バッファ */
    int     n;                      /* 受信バイト数 */

    /* 転送先の設定 */
    char *forward_host = "node1";   /* 転送先ホスト名 */
    char *forward_port = "10000";   /* 転送先ポート番号 */

    /* 受信ポート設定 */
    unsigned short listen_port = UDP_SERVER_PORT;  /* デフォルトは10000 */

    struct in_addr addr;            /* アドレス表示用 */
    struct addrinfo hints, *res;
    int err;

    /* コマンドライン引数の処理 */
    if(argc == 2 && strncmp(argv[1], "-h", 2) == 0) {
        printf("Usage: %s [listen_port] [forward_host] [forward_port]\n", argv[0]);
        printf("       default listen port: %d\n", UDP_SERVER_PORT);
        printf("       default forward host: %s\n", forward_host);
        printf("       default forward port: %s\n", forward_port);
        return 0;
    }

    /* オプション: コマンドライン引数での設定 */
    if(argc >= 2) {
        listen_port = (unsigned short)atoi(argv[1]);
    }
    if(argc >= 3) {
        forward_host = argv[2];
    }
    if(argc >= 4) {
        forward_port = argv[3];
    }

    printf("=== UDP Relay Server ===\n");
    printf("Listen port: %d\n", listen_port);
    printf("Forward to: %s:%s\n", forward_host, forward_port);
    printf("========================\n\n");

    /* ========================================
     * 受信用ソケットの設定
     * ======================================== */

    /* UDPソケットをオープンする */
    from_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(from_sock < 0) {
        perror("from_socket");
        return 1;
    }

    /* サーバアドレス構造体の設定 */
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* すべてのインターフェースで受信 */
    serverAddr.sin_port = htons(listen_port);

    /* ソケットにアドレスをバインド */
    if(bind(from_sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        close(from_sock);
        return 1;
    }

    printf("Waiting for data on port %d...\n", listen_port);

    /* ========================================
     * 転送先の解決と転送用ソケットの設定
     * ======================================== */

    /* ホスト名からIPアドレスを検索する */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          /* IPv4で通信 */
    hints.ai_socktype = SOCK_DGRAM;     /* UDP */

    err = getaddrinfo(forward_host, forward_port, &hints, &res);
    if(err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        close(from_sock);
        return 1;
    }

    /* 確認用：解決したIPアドレスを文字列に変換して表示 */
    addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    printf("Forward destination IP: %s\n", inet_ntoa(addr));
    printf("Forward destination port: %d\n\n",
           ntohs(((struct sockaddr_in*)(res->ai_addr))->sin_port));

    /* 転送用UDPソケットをオープンする */
    to_sock = socket(res->ai_family, res->ai_socktype, 0);
    if(to_sock < 0) {
        perror("to_socket");
        freeaddrinfo(res);
        close(from_sock);
        return 1;
    }

    /* ========================================
     * メインループ：受信して転送
     * ======================================== */

    addrLen = sizeof(clientAddr);

    int cnt = 0;
    while(1) {
        /* データを受信 */
        n = recvfrom(from_sock, buf, BUF_LEN, 0,
                     (struct sockaddr *)&clientAddr, &addrLen);

        if(n < 0) {
            perror("recvfrom");
            break;
        }

        if(n == 0) {
            continue;  /* データなし */
        }

        cnt += n;
        /* 受信したデータを表示 *//*
        printf("Received %d bytes from %s:%d\n",
               n,
               inet_ntoa(clientAddr.sin_addr),
               ntohs(clientAddr.sin_port));*/
        //printf("%d\n", cnt);

        /* 転送先にデータを送信 */
        if(sendto(to_sock, buf, n, 0, res->ai_addr, res->ai_addrlen) != n) {
            perror("sendto");
            break;
        }

        //printf("Forwarded %d bytes to %s:%s\n\n", n, forward_host, forward_port);
    }

    /* クリーンアップ */
    freeaddrinfo(res);
    close(to_sock);
    close(from_sock);

    return 0;
}
