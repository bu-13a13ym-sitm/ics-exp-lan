
#include "icslab2_net.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

int main(int argc, char **argv){
    char *port_num_str = UDP_SERVER_PORT_STR;
    char *server_ipaddr_str ="172.21.0.30";/*サーバアドレスIP */
    unsigned int port = UDP_SERVER_PORT;/*ポート番号*/
    int sock;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int addrLen;
    char buf[BUF_LEN];
    int n;
    char request = '0';
    int fd;
    struct timespec start_time, end_time;
    unsigned int sec;
    int nsec;
    double time;
    int err;

    /*コマンドライン引数の処理*/
    if (argc !=3){
        printf("Usage:%s <output_filename> <num_target_node>\n", argv[0]);
        return 0;
    }
    clock_gettime(CLOCK_REALTIME,&start_time);

    char *output_filename = argv[1];
    printf("Output filename:%s\n", output_filename);
    /* STEP 0:出力ファイルのオープン*/
    fd = open(output_filename, O_CREAT | O_WRONLY | O_TRUNC,0644);
    if (fd <0){
        perror("open");
        return 1;
    }

    int num_target_node = (unsigned int)atoi(argv[2]);
    char target_node[16];
    sprintf(target_node, "node%d", num_target_node);
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
    
    /* STEP 2:ソケットをオープンするUDP */
    sock = socket(res->ai_family, res->ai_socktype, 0);
    if (sock < 0){
        perror("socket");
        close(fd);
        return 1;
    }
    /* STEP 3:サーバにリクエストとして’O’を送信する*/
    if (sendto(sock,&request,2, 0,res->ai_addr, res->ai_addrlen) != 2){
        perror("sendto");
        close(sock);
        close(fd);
        return 1;
    }
    printf("Sent request:'0'\n");

    
    /* STEP 2:ソケットをオープンするUDP */
    int rsock = socket(AF_INET, SOCK_DGRAM,0);
    if (rsock < 0){
        perror("socket");
        close(fd);
        return 1;
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* すべてのインターフェースで受信 */
    serverAddr.sin_port = htons(port);

    /* ソケットにアドレスをバインド */
    if(bind(rsock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        close(sock);
        close(rsock);
        return 1;
    }

    /* STEP 4: サーバからのファイル転送データを受け取ってファイルに書き込む*/
    addrLen = sizeof(clientAddr);
    int cnt = 0;
    while ((n = recvfrom(rsock, buf, BUF_LEN, 0, (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen)) > 0) {
        cnt += n;
        if (n < 0) {
            perror("recvfrom");
        }
        //printf("%d\n", cnt);
        if (write(fd, buf, n) != n) {
            perror("write");
            close(sock);
            close(rsock);
            close(fd);
            return 1;
        }
        clock_gettime(CLOCK_REALTIME, &end_time);
    }
    sec = end_time.tv_sec- start_time.tv_sec;
    nsec = end_time.tv_nsec- start_time.tv_nsec;
    time = (double)sec + (double)nsec * 1e-9;
    printf("File received and saved to %s\n", output_filename);
    printf("file size : %d\n", cnt);
    printf("time : %f sec\n", time);
    printf("throughput : %f bps\n", cnt * 8 / time);
    /* STEP 5: ソケットとファイルをクローズする*/
    close(sock);
    close(rsock);
    close(fd);
    return 0;
}