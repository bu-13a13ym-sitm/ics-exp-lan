#include "icslab2_net.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char** argv){
int sock;
struct sockaddr_in serverAddr;
struct sockaddr_in clientAddr;
int addrLen;
char buf[BUF_LEN];
int n;
char *filename;
int fd;

if (argc !=2){
printf("Usage:%s <filename>\n", argv[0]);
return 0;
}

filename = argv[1];
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
 n = recvfrom(sock, buf, BUF_LEN, 0, (struct sockaddr *)&clientAddr, (socklen_t
 *)&addrLen);
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
 while ((n = read(fd, buf, BUF_LEN)) > 0) {
 cnt += n;
 usleep(300);
 if (sendto(sock, buf, n, 0, (struct sockaddr *)&clientAddr, addrLen) != n) {
 perror("sendto");
 break;
 } else printf("%d\n", cnt);
}

//ファイル送信ループの後、終了メッセージを送信
sendto(sock,"",0,0,(struct sockaddr *)&clientAddr, sizeof(clientAddr));
close(fd);
}
}
close(sock);
return 0;
}