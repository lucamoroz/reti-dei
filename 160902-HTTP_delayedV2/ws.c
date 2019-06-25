#include<stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

struct header{
  char * n;
  char * v;
}h[100];

FILE * f;

struct sockaddr_in myaddr,remote_addr;

char  filename[100],command[1000];
char request[10000];
char response[10000];
int yes=1;

int main() {
  char *method, *path, *ver;
  int c;
  int duepunti;
  int i,k,j,n,s,s2;
  int length, lungh;

  s = socket(AF_INET, SOCK_STREAM,0);
  if ( s == -1) {
    perror("socket fallita");
    return 1;
  }
  if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {perror("setsockopt fallita"); return 1; }

  myaddr.sin_family=AF_INET;
  myaddr.sin_port=htons(9577);

  if (bind(s,(struct sockaddr *)&myaddr,sizeof(struct sockaddr_in))==-1){ perror("bind fallita"); return 1; }
  if (listen(s,10)==-1){ perror("Listen Fallita");return 1; }

  lungh=sizeof(struct sockaddr_in);

  // todo support > 1 
  unsigned int cached_ip = 0;

  while(1){
    s2 = accept(s,(struct sockaddr *) &remote_addr,&lungh);
    if (s2 == -1) {perror("accept fallita"); return 1;}

    // parse headers
    j=0;k=0;
    h[0].n=request;
    while(read(s2,request+j,1)){
      if((request[j]=='\n') && (request[j-1]=='\r')){
        printf("LF\n");
        request[j-1]=0;
        duepunti=0;
        if(h[k].n[0]==0) break;
        h[++k].n=request+j+1;
      }
      if(request[j]==':' && (!duepunti)){
        duepunti=1;
        request[j]=0;
        h[k].v=request+j+1;
      }
      j++;
    }
    if (k==0) {close(s2);continue;}

    //perror("Read\n");
    printf("Command-Line: %s\n",h[0].n);
    printf("===== HEADERS ==============\n");
    for(i=1;i<k;i++){
      printf("%s --> %s\n", h[i].n, h[i].v);
    }

    // parse command line
    // GET /qui/quo/qua.xxx HTTP/1.1
    method = h[0].n;
    for(i=0;h[0].n[i]!=' ';i++); h[0].n[i]=0;i++;
    for(;h[0].n[i]==' ';i++);
    path = &h[0].n[i];
    for(;h[0].n[i]!=' ';i++); h[0].n[i]=0; i++;
    for(;h[0].n[i]==' ';i++);
    ver = &h[0].n[i];

    printf("\n%s %s %s\n",method,path,ver);

    // get path: if not available return default content otherwise
    // send a response with retry-after header saving the requesting ip and at
    // the successive req send the requested content

    strcpy(filename,path+1);
    f=fopen(filename,"r");
    if (f == NULL) {
      // send default
      f = fopen("default.html", "r");
      sprintf(response, "HTTP/1.1 404 Not Found\r\nConnection:close\r\n\r\n");
      write(s2, response, strlen(response));
      while ((c = fgetc(f)) != EOF)
        write(s2, &c, 1);

      close(s2);
      continue;
    }

    if (cached_ip != remote_addr.sin_addr.s_addr) {
      printf("\nFirst try of:  ");
      for (i=0; i<4; i++) printf("%d\n", ((unsigned char*)&remote_addr.sin_addr.s_addr)[i]);

      // tell client browser to retry after 2 seconds
      sprintf(response, "HTTP/1.1 307 Temporary Redirect\r\nConnection:close\r\nRetry-After:2\r\nContent-Type:text/html\r\n\r\nPlease wait a moment...\r\n\r\n");
      write(s2, response, strlen(response));
      cached_ip = remote_addr.sin_addr.s_addr;
      close(s2);
      continue;
    } else {
      f = fopen(filename, "r");
      sprintf(response, "HTTP/1.1 200 OK\r\nConnection:close\r\nContent-Type:text/html\r\n\r\n");
      write(s2, response, strlen(response));
      while ((c = fgetc(f)) != EOF)
        write(s2, &c, 1);
      cached_ip = 0; // just for testing
      close(s2);
      continue;
    }
  }
  close(s);
}
