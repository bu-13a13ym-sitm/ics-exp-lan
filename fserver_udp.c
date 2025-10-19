#include "icslab2_net.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char** argv) {
    char *port_num_str = UDP_SERVER_PORT_STR;
    int sock;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int addrLen;
    char buf[BUF_LEN];
    int n;
    char *filename;
    int fd;
    int this;

    int err;

    if (argc !=3){
        printf("Usage:%s <filename> <num this node>\n", argv[0]);
        return 0;
    }

    filename = argv[1];
    this = atoi(argv[2]);
    printf("Filename:%s\n", filename);

    /* STEP 1:ソケットをオープンするUDP */
    if ((sock = socket(AF_INET, SOCK_DGRAM,0))<0){
        perror("socket");
        return 1;
    }
    /* STEP 2:受け付けるアドレスとポートを設定するIP */
    memset(&serverAddr,0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(UDP_SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* STEP 3: ソケットとアドレスをするbind */
    if (bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }
    for (;;) {
        /* STEP 4: クライアントからのリクエストを待つ*/
        addrLen = sizeof(clientAddr);
        n = recvfrom(sock, buf, BUF_LEN, 0, (struct sockaddr *)&clientAddr, (socklen_t  *)&addrLen);
        if (n < 0) {
            perror("recvfrom");
            break;
        }
        buf[n] = '\0';
        if (buf[0] == '0') { /* ’O’ のリクエストを受け取ったら*/
            printf("Received request:'0'\n");
            /* STEP 5: ファイルを読み込み、クライアントに送信する*/
            fd = open(filename, O_RDONLY);
            if (fd < 0) {
                perror("open");
                close(sock);
                return 1;
            }
            int cnt = 0;
            int sockets[4];
            struct addrinfo* addrs[4];
            for (int i = 1; i <= 4; i++) {
                int target_node_num = i;
                if (i >= this) target_node_num++;

                char target_node[16];
                sprintf(target_node, "node%d", target_node_num);
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
                addrs[i - 1] = res;
                
                sockets[i - 1] = socket(res->ai_family, res->ai_socktype, 0);
                if(sockets[i - 1] < 0) {
                    perror("socket");
                    return  1;
                }
            }
            int num = 0;
            while ((n = read(fd, buf, BUF_LEN)) > 0) {
                int curr_sock = sockets[num % 4];
                struct addrinfo* curr_addr = addrs[num % 4];
                cnt += n;
                //usleep(300);
                if (sendto(curr_sock, buf, n, 0, curr_addr->ai_addr, curr_addr->ai_addrlen) != n) {
                    perror("sendto");
                    break;
                }// else printf("%d\n", cnt);
                num++;
            }
            printf("fin\n");
            sleep(1);
            for (int i = 0; i < 4; i++) {
                int curr_sock = sockets[i];
                struct addrinfo* curr_addr = addrs[i];
                sendto(curr_sock, "0", 1, 0, curr_addr->ai_addr, curr_addr->ai_addrlen);
                close(curr_sock);
            }
            close(fd);
        }
    }
    close(sock);
    return 0;
}