#include<stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


struct header{
  char * n;
  char * v;
}h[100];

char request[1000];
char response[20000];
char entity_body[200000];
struct sockaddr_in server;

//unsigned char ip[4]={216,58,212,99}; //google
//unsigned char ip[4]={17,172,224,111}; //apple
//unsigned char ip[4]={127,0,0,1};  //Me stesso
unsigned char ip[4]={216,58,213,228};

int main()
{
  int length=-1,iter;
  int n,s,i,j,k;
  FILE * f;
  int duepunti;
  s=socket(AF_INET, SOCK_STREAM, 0);
  if(s==-1){
    perror("socket fallita");
    return 1;
  }
  server.sin_family = AF_INET;
  server.sin_port = htons(80);
  server.sin_addr.s_addr = *(unsigned int *)ip;

  if ( connect(s, (struct sockaddr *)&server,sizeof(struct sockaddr_in))==-1){
    perror("Connect fallita");
    return 1;
  }

  sprintf(request,"GET /?gfe_rd=cr&dcr=0&ei=7qmvWvjcMdfEaM21i4AK HTTP/1.1\r\nHost:www.google.co.uk\r\nConnection:close\r\n\r\n");
  write(s,request,strlen(request));
  j=0;k=0;
  h[k].n = response;
  duepunti=0;
  while(read(s,response+j,1)){
    printf("%c",response[j]);
    if((response[j]=='\n') && (response[j-1]=='\r')){
      response[j-1]=0;
      duepunti=0;
      if(h[k].n[0]==0) break;
      h[++k].n=response+j+1;
    }
    if(response[j]==':' && (!duepunti)){
      duepunti=1;
      response[j]=0;
      h[k].v=response+j+1;
    }
    j++;
  }
  //printf("Command-Line: %s\n",h[0].n);
  printf("===== HEADERS ==============\n");
  for(i=1;i<k;i++){
    printf("%s --> %s\n", h[i].n, h[i].v);
    if(!strcmp(h[i].n,"Content-Length"))
      length = atoi(h[i].v);
  }

  if (length == -1) {
    printf("%s\n", "\nLength not provided.\n");
    close(s);
    return 0;
  }
  printf("Content size: %d\n", length);

  char *content = (char*)malloc(length * sizeof(char));

  for (i=0; n = read(s, content + i, length - i); i += n);

  if (i != length) {
    printf("Error reading content!\n");
    return 1;
  }

  for (i=0; i < length; i++) {
    printf("%c\n", content[i]);
  }

  free(content);
  close(s);
}
