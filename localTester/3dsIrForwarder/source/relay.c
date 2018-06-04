#include <3ds.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "main.h"
#include "relay.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"


#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
#define WIFI_PORT       1869


bool                      relayInited = false;
bool                      clientBound;
bool                      irInited;
bool                      socketInited;
static u32*               irBuffer;
static u32*               socketBuffer;
//static s32                sock;
//static s32                csock;
//static struct sockaddr_in client;
//static struct sockaddr_in server;
//static u32                clientLen = sizeof(client);

static bool mbedTlsInited = false;
static mbedtls_net_context bindFd;
static mbedtls_net_context clientFd;
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctrDrbg;
static mbedtls_ssl_context ssl;
static mbedtls_ssl_config conf;
const char* sendCertFile = "sdmc:/3dsIrForwarder/send.pem";
const char* receiveCertFile = "sdmc:/3dsIrForwarder/receive.pem";

static const char loginQuery[] = "I am a Palm OS command transmitter.";
static const char loginAnswer[] = "I want to login to a Palm OS Device!";


static inline int8_t makeCodeFromBaud(uint32_t baud){
   switch(baud){
      case 115200:
         return 3;
      case 96000:
         return 4;
      case 72000:
         return 5;
      case 57600:
         return 14;
      case 48000:
         return 6;
      case 38400:
         return 15;
      case 36000:
         return 7;
      case 24000:
         return 8;
      case 19200:
         return 16;
      case 18000:
         return 9;
      case 12000:
         return 10;
      case 9600:
         return 11;
      case 7200:
         return 17;
      case 6000:
         return 12;
      case 4800:
         return 18;
      case 3000:
         return 13;
   }
   
   return -1;
}

static void cleanup(){
   if(mbedTlsInited){
      mbedtls_net_free(&server_fd);
      mbedtls_ssl_free(&ssl);
      mbedtls_ssl_config_free(&conf);
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      mbedTlsInited = false;
   }
   if(irInited)
      iruExit();
   if(socketInited)
      socExit();
   if(irBuffer)
      free(irBuffer);
   if(socketBuffer)
      free(socketBuffer);
}

static void mbedTlsCustomDebug(void *ctx, int level, const char *file, int line, const char *str){
   //((void) level);
   //fprintf( (FILE *) ctx, "%s:%04d: %s", file, line, str );
   //fflush(  (FILE *) ctx  );
   printf("%s:%04d: %s", file, line, str);
}

uint32_t relayInit(uint32_t irBaud){
   if(!relayInited){
      int8_t irBaudCode = makeCodeFromBaud(irBaud);
      Result systemCallValue;
      int ret;
      
      if(irBaudCode == -1)
         return RELAY_ERROR_INVALID_PARAM;
      
      irInited = false;
      socketInited = false;
      clientBound = false;
      
      mbedtls_net_init(&bindFd);
      mbedtls_net_init(&clientFd);
      mbedtls_ssl_init(&ssl);
      mbedtls_ssl_config_init(&conf);
      mbedtls_x509_crt_init(&cacert);
      mbedtls_ctr_drbg_init(&ctrDrbg);
      mbedtls_entropy_init(&entropy);
      mbedTlsInited = true;
      
      ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers));
      if(ret != 0){
         printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
         return false;
         //goto exit;
      }
      
      ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
      if(ret != 0){
         return false;
      }
      
      //mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
      
      mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctrDrbg);
      //mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
      mbedtls_ssl_conf_dbg(&conf, mbedTlsCustomDebug);
      mbedtls_ssl_set_bio(&ssl, &clientFd, mbedtls_net_send, mbedtls_net_recv, NULL);
      
      socketBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
      irBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
      if(!irBuffer || !socketBuffer){
         cleanup();
         return RELAY_ERROR_OUT_OF_MEMORY;
      }
      
      systemCallValue = iruInit(irBuffer, SOC_BUFFERSIZE);
      if(systemCallValue != RL_SUCCESS){
         cleanup();
         return RELAY_ERROR_IR_DRIVER;
      }
      irInited = true;
      
      systemCallValue = IRU_SetBitRate((uint8_t)irBaudCode);
      if(systemCallValue != RL_SUCCESS){
         cleanup();
         return RELAY_ERROR_IR_DRIVER;
      }
      
      systemCallValue = socInit(socketBuffer, SOC_BUFFERSIZE);
      if(systemCallValue != RL_SUCCESS){
         cleanup();
         return RELAY_ERROR_WIFI_DRIVER;
      }
      socketInited = true;
      
      /*
      sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
      if(sock < 0){
         cleanup();
         return RELAY_ERROR_CANT_OPEN_SOCKET;
      }
      
      memset(&server, 0, sizeof(server));
      memset(&client, 0, sizeof(client));
      
      server.sin_family = AF_INET;
      server.sin_port = htons(80);
      server.sin_addr.s_addr = gethostid();
      
      ret = bind(sock, (struct sockaddr*)&server, sizeof(server));
      if(ret != 0){
         cleanup();
         return RELAY_ERROR_CANT_BIND_SOCKET;
      }
      */
      
      //need to convert this devices ip to a string
      ret = mbedtls_net_bind(&bindFd, const char *bind_ip, const char *port, MBEDTLS_NET_PROTO_TCP);
      if(ret != 0){
         cleanup();
         return RELAY_ERROR_CANT_BIND_SOCKET;
      }
      
      ret = mbedtls_net_set_nonblock(&bindFd);
      if(ret != 0){
         cleanup();
         mbedtls_net_free(&bindFd);
         return RELAY_ERROR_CANT_SET_NONBLOCKING;
      }
      
      relayInited = true;
   }
   
   return RELAY_ERROR_NONE;//already inited returns success
}

void relayExit(){
   if(relayInited){
      cleanup();
      mbedtls_net_free(&bindFd);
      if(clientBound)
         mbedtls_net_free(&clientFd);
      relayInited = false;
   }
}

uint32_t relayAttemptConnection(){
   if(relayInited){
      uint8_t commandBuffer[sizeof(loginAnswer)] = {0};
      uint32_t bytes;
      int ret;
      
      if(clientBound){
         mbedtls_net_free(&clientFd);
         clientBound = false;
      }

      ret = mbedtls_net_accept( mbedtls_net_context *bind_ctx, mbedtls_net_context *client_ctx, void *client_ip, size_t buf_size, size_t *ip_len );
      if(ret != 0)
         return RELAY_ERROR_NO_CLIENTS;
      clientBound = true;
      
      ret = mbedtls_net_set_nonblock(&clientFd);
      if(ret != 0)
         return RELAY_ERROR_CANT_SET_NONBLOCKING;
      
      //check if this is an internet scanner bot or a login
      mbedtls_ssl_write(&ssl, loginQuery, strlen(loginQuery), 0);
      
      //check if responce matches
      safeSleep(3);
      bytes = mbedtls_ssl_read(&ssl, commandBuffer, sizeof(loginAnswer), 0);
      if(bytes != sizeof(loginAnswer)){
         cleanup();
         return RELAY_ERROR_TIMEOUT;
      }
      
      if(memcmp(commandBuffer, loginAnswer, sizeof(loginAnswer)) != 0){
         //responce is wrong, this is a scanner, DDOS(very unlikely) or clueless web user(most likely, they tried to open the forwarder as a web page)
         mbedtls_net_free(&clientFd);
         return RELAY_ERROR_INVALID_PARAM;
      }
      
      return RELAY_ERROR_NONE;
   }

   return RELAY_ERROR_NOT_INITED;
}

uint32_t relayGetMyIp(){
   return gethostid();
}

void relayRun(){
   //simply copys all data from WIFI to IR and IR to WIFI
   uint8_t swapBuffer[1024];
   uint32_t bytes;
   
   //read from WIFI
   bytes = mbedtls_ssl_read(&ssl, swapBuffer, sizeof(swapBuffer), 0);
   //forward to IR
   iruSendData(swapBuffer, bytes, false/*wait*/);
   
   //read from IR
   iruRecvData(swapBuffer, sizeof(swapBuffer), 0/*flag*/, &bytes, false/*wait*/);
   //forward to WIFI
   mbedtls_ssl_write(&ssl, swapBuffer, bytes, 0);
}
