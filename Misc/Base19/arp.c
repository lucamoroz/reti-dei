#include<stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <string.h>


unsigned char miomac[6] = { 0xf2,0x3c,0x91,0xdb,0xc2,0x98 };
unsigned char broadcast[6] = { 0xff, 0xff, 0xff, 0xff,0xff,0xff};
unsigned char mioip[4] = {88,80,187,84};

unsigned char iptarget[4] = {88,80,187,80};
struct sockaddr_ll sll;

struct eth_frame {            // TOT 14 bytes
  unsigned char dst[6];
  unsigned char src[6];
  unsigned short int type;    // 0x0800 -> IP, 0x0806 -> ARP
  unsigned char payload[1];
};

struct arp_packet{            // TOT 28 bytes (with ip and eth protocols)
  unsigned short int htype;   // 1 -> eth
  unsigned short int ptype;   // 0x0800 -> ip
  unsigned char hlen;
  unsigned char plen;
  unsigned short int op;      // operation: request -> 1, response -> 2
  unsigned char hsrc[6];
  unsigned char psrc[4];
  unsigned char hdst[6];
  unsigned char pdst[4];
};

unsigned char buffer[1500];

void stampa_buffer( unsigned char* b, int quanti);

void crea_eth(struct eth_frame * e , unsigned char * dest, unsigned short type){
  int i;
  for(i=0;i<6;i++){
    e->dst[i]=dest[i];
    e->src[i]=miomac[i];
  }
  e->type=htons(type);
}

void crea_arp( struct arp_packet * a, unsigned short op,  unsigned char *ptarget) {
  int i;
  a->htype=htons(1);
  a->ptype=htons(0x0800);
  a->hlen=6;
  a->plen=4;
  a->op=htons(op);
  for(i=0;i<6;i++){
    a->hsrc[i]=miomac[i];
    a->hdst[i]=0;
  }
  for(i=0;i<4;i++){
    a->psrc[i]=mioip[i];
    a->pdst[i]=ptarget[i];
  }
}


int risolvi(unsigned char* target, unsigned char * mac_incognito) {
  struct eth_frame * eth;
  struct arp_packet * arp;
  int i,n,s;
  int lungh;
  s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if(s==-1) { perror("socket fallita"); return 1; }

  eth = (struct eth_frame*) buffer;
  arp = (struct arp_packet *) eth->payload;

  crea_arp(arp,1,target);
  crea_eth(eth,broadcast,0x0806);
  //stampa_buffer(buffer, 14+sizeof(struct arp_packet));

  sll.sll_family=AF_PACKET;
  sll.sll_ifindex=3;
  lungh=sizeof(sll);
  // 14: eth frame size
  n = sendto(s, buffer, 14 + sizeof(struct arp_packet), 0, (struct sockaddr *) &sll, lungh);
  if(n==-1) { perror("sendto fallita"); return 1;}

  while( 1 ) {
    n = recvfrom(s, buffer, 1500, 0, (struct sockaddr *) &sll, &lungh);
    if(n==-1) { perror("recvfrom fallita"); return 1;}
    if (eth->type ==htons(0x0806))
      if(arp->op ==htons(2))
        if(!memcmp(arp->psrc,target,4)){
          for(i=0;i<6;i++)
          mac_incognito[i]=arp->hsrc[i];
          break;
        }
  }
  return 0;
}


void stampa_buffer( unsigned char* b, int quanti){
  int i;
  for(i=0;i<quanti;i++){
    printf("%.3d(%.2x) ",b[i],b[i]);
    if((i%4)==3)
    printf("\n");
  }
}


int main(){
  int t;
  unsigned char mac[6];
  t=risolvi(iptarget,mac);
  if  (t==0){
    printf("Mac incognito:\n");
    stampa_buffer(mac,6);
  }
}
