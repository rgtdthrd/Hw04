/* hw4-client.c */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

int main()
{
  #if 1
  /* create TCP client socket (endpoint) */
  int sd = socket( AF_INET, SOCK_STREAM, 0 );
  if ( sd == -1 ) { perror( "socket() failed" ); exit( EXIT_FAILURE ); }

  //struct hostent * hp = gethostbyname( "linux02.cs.rpi.edu" );
  struct hostent * hp = gethostbyname( "localhost" );
  #endif
  #if 0
      struct addrinfo hints;
    struct addrinfo *result, *hp;
    int sd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;      /* Allow IPv4 */
    hints.ai_socktype = SOCK_STREAM; /* TCP socket */
    hints.ai_flags = 0;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    int s = getaddrinfo("linux04.cs.rpi.edu", "8123", &hints, &result);
    if (s != 0) 
    {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
      exit(EXIT_FAILURE);
    }
  #endif

  /* TO DO: rewrite the code above to use getaddrinfo() */

  if ( hp == NULL )
  {
    fprintf( stderr, "ERROR: gethostbyname() failed\n" );
    return EXIT_FAILURE;
  }

  struct sockaddr_in tcp_server;
  tcp_server.sin_family = AF_INET;  /* IPv4 */
  memcpy( (void *)&tcp_server.sin_addr, (void *)hp->h_addr, hp->h_length );
  unsigned short server_port = 8192;
  tcp_server.sin_port = htons( server_port );

  printf( "CLIENT: TCP server address is %s\n", inet_ntoa( tcp_server.sin_addr ) );

  printf( "CLIENT: connecting to server...\n" );

  if ( connect( sd, (struct sockaddr *)&tcp_server, sizeof( tcp_server ) ) == -1 )
  {
    perror( "connect() failed" );
    return EXIT_FAILURE;
  }


  /* The implementation of the application protocol is below... */

while ( 1 )    /* TO DO: fix the memory leaks! */
{
  char * buffer = calloc( 9, sizeof( char ) );
#if 0
  printf("buffer: 0x%p, buffer+1: 0x%p\n", buffer, buffer+1);
  printf("buffer: 0x%p, buffer+1: 0x%p\n", (unsigned int *)(buffer), (unsigned int *)(buffer)+1);
  printf("buffer: 0x%p, buffer+1: 0x%p\n", (unsigned int *)(buffer), (unsigned int *)(buffer+1));
#endif
  if ( fgets( buffer, 9, stdin ) == NULL ) break;
  if ( strlen( buffer ) != 6 ) { printf( "CLIENT: invalid -- try again\n" ); continue; }
  *(buffer + 5) ='\0';   /* get rid of the '\n' */

  printf( "CLIENT: Sending to server: %s\n", buffer );
  int n = write( sd, buffer, strlen( buffer ) );    /* or use send()/recv() */
  if ( n == -1 ) { perror( "write() failed" ); return EXIT_FAILURE; }

  n = read( sd, buffer, 9 );    /* BLOCKING */

  if ( n == -1 )
  {
    perror( "read() failed" );
    return EXIT_FAILURE;
  }
  else if ( n == 0 )
  {
    printf( "CLIENT: rcvd no data; TCP server socket was closed\n" );
    break;
  }
  else /* n > 0 */
  {
    switch ( *buffer )
    {
      case 'N': printf( "CLIENT: invalid guess -- try again" ); break;
      case 'Y': printf( "CLIENT: response: %s", buffer + 3 ); break;
      default: break;
    }

    short guesses = ntohs( *(short *)(buffer + 1) );
    printf( " -- %d guess%s remaining\n", guesses, guesses == 1 ? "" : "es" );
    //if the result does not contain '-' then the word is guessed
    int flag =0;
    printf("string size: %ld\n", strlen((const char*)(buffer+3)));
    for (int i=0; i<5; i++){
      if (*(buffer+3+i) == '-' || *(buffer+3+i) == '?'){
        flag = -1;
        break;
      }
    }
    if (flag == 0){
      printf( "CLIENT: word guessed!\n" );
      break;
    }
    if ( guesses == 0 ) break;
  }
}


  printf( "CLIENT: disconnecting...\n" );

  close( sd );

  return EXIT_SUCCESS;
}
