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
#include <pthread.h>
#include <ctype.h>
#include <signal.h>

extern int total_guesses;
extern int total_wins;
extern int total_losses;
extern char ** words;
char* selected_word;
int seed;
int num_words;
int port;
char * filename;
pthread_mutex_t lock;
void signal_handler( int sig )
{

	printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
    pthread_mutex_destroy(&lock);
    printf(" Wordle server shutting down...\n\n");
    printf(" guesses: %d\n", total_guesses);
    printf(" wins: %d\n", total_wins);
    printf(" losses: %d\n\n", total_losses);
    printf(" word(s) played: %s\n", toUpperCase(selected_word));

    //exit with success
    exit(EXIT_SUCCESS);
}
char* toUpperCase(char *str) {
    char *p = calloc(6, sizeof(char));
    for (int i = 0; i < 5; i++) {
        *(p+i) = toupper(*(str+i));
    }
    return p;
}
void toLowerCase(char *str) {
    for (int i = 0; i < 5; i++) {
        *(str+i) = tolower(*(str+i));
    }
}
int find_word(char * word){
    for (int i = 0; i < num_words; i++){
        if (strcmp(word, *(words+i)) == 0){
            return 1;
        }
    }
    return 0;
}
char* check_words(char* expected, char* guess){
    char* reply = calloc(6, sizeof(char));
    *(reply+5) = '\0';
    for (int i = 0; i < 5; i++){
        int in = 0;
        for (int j = 0; j < 5; j++){
            if (*(expected+j) == *(guess+i)){
                in = 1;
                break;
            }
        }
        if (in == 1){
            *(reply+i) = *(guess+i);
        }else{
            *(reply+i) = '-';
        }
    }
    for (int i =0;i < 5; i++){
        if (*(expected+i) == *(guess+i)){
            // make the letter uppercase
            *(reply+i) = *(reply+i)-32;
        }
    }
    return reply;
}
void error_handler(){
    fprintf(stderr, "USAGE: hw4.out <listener-port> <seed> <word-filename> <num-words>\n");
    exit(EXIT_FAILURE);
}
void* thread_func(void * arg){
    int newsd = *(int*)arg;
    
    
    int n = 0;
    int flag = 0;
    short guesses = 0;
    char* buffer = calloc(6, sizeof(char));
    do{ 
        char* msg = calloc(9, sizeof(char));
        printf("THREAD %ld: waiting for guess\n",pthread_self());
        n = recv( newsd, buffer, 6, 0 );
        if (n == 0){
            free(msg);
            continue;
        }
        toLowerCase(buffer);
        *(buffer+5) = '\0';
        printf("THREAD %ld: rcvd guess: %s\n",pthread_self(), buffer);
        int found = find_word(buffer);
        if (found == 0){
            printf("THREAD %ld: invalid guess; sending reply: ????? (%d guesses left)\n",pthread_self(), 6-guesses);
            strcpy(msg, "N");
            short g = htons(6-guesses);
            memcpy(msg+1, &g, 2);
            memcpy(msg+3, "?????", 5);
            send(newsd, msg, 9, 0);
        }else{
            guesses++;
            char* reply = check_words(selected_word, buffer);
            printf("THREAD %ld: sending reply: %s (%d guesses left)\n",pthread_self(), reply, 6-guesses);
            strcpy(msg, "Y");
            short g = htons(6-guesses);
            memcpy(msg+1, &g, 2);
            strcpy(msg+3, reply);
            send(newsd, msg, 9, 0);
            free(reply);
        }
        if (strcmp(buffer, selected_word) == 0){
            flag = 1;
        }
        //clear buffer
        memset(buffer, 0, 10);
        //clear msg
        memset(msg, 0, 9);
    }while(n>0 && flag == 0 && guesses < 6);
    if (flag == 1 || guesses == 6){
        char* ret = toUpperCase(selected_word);
        printf("THREAD %ld: game over; word was %s!\n",pthread_self(), ret);
        free(ret);
    }else{
        printf("THREAD %ld: client gave up; closing TCP connection...\n",pthread_self());
    }
    pthread_mutex_lock(&lock);
    if (flag == 1){
        total_wins++;
    }else{
        total_losses++;
    }
    total_guesses += (int)guesses;
    pthread_mutex_unlock(&lock);

    free(buffer);
    close(newsd);
    //thread exit  
    return NULL;
}

int wordle_server( int argc, char ** argv ){
    //signal handling
    // signal(SIGINT, SIG_IGN);
    // signal(SIGTERM, SIG_IGN);
    // signal(SIGUSR2, SIG_IGN);

    signal( SIGUSR1, signal_handler);
    //signal( SIGINT, signal_handler);
    setvbuf(stdout, NULL, _IONBF, 0);
    //assume argc = 5 USAGE: hw4.out <listener-port> <seed> <word-filename> <num-words>
    if(argc != 5){
        error_handler();
    }
    //initialize variables
    port = atoi(*(argv+1));
    seed = atoi(*(argv+2));
    filename = *(argv+3);
    num_words = atoi(*(argv+4));
    if (port <1 || seed <0 || num_words < 0){
        error_handler();
    }
    words = realloc(words, (num_words+1) * sizeof(char *));
    if(words == NULL){
        error_handler();
    }
    //open file
    FILE * file = fopen(filename, "r");
    if(file == NULL){
        perror("fopen() failed");
        return EXIT_FAILURE;
    }
    printf("MAIN: opened %s (%d words)\n", filename, num_words);
    //read words from file
    for(int i = 0; i < num_words; i++){
        *(words+i) = calloc(6, sizeof(char));
        if(fscanf(file, "%s", *(words+i)) != 1){
            perror("fscanf() failed");
            return EXIT_FAILURE;
        }
        //manually set the end of the word to null
        *(*(words+i)+5) = '\0';
    }
    //for loop the words and test all words are loaded
    // for ( char ** ptr = words ; *ptr ; ptr++ ){
    // printf( "WORD: %s\n", *ptr );
    // }
    fclose(file);

    //seed the random number generator
    srand(seed);
    int w = rand() % num_words;
    selected_word = *(words+w);

    int sd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket at our IP address.
    if (sd == -1){
        perror("socket() failed");
        return EXIT_FAILURE;
    }
    struct sockaddr_in server;
    socklen_t length = sizeof(server);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl( INADDR_ANY );
    if (bind(sd, (struct sockaddr *) &server, sizeof(server)) < 0){
        perror("bind() failed: ");
        return EXIT_FAILURE;
    }
    struct sockaddr_in client;
    if (listen(sd, 5) < 0){
        perror("listen() failed: ");
        return EXIT_FAILURE;
    }
    printf("MAIN: Wordle server listening on port {%d}\n",port);
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("Mutex init has failed\n");
        return 1;
    }
    
    while(1){
        int newsd = accept(sd, (struct sockaddr *) &client, &length);
        printf("MAIN: rcvd incoming connection request\n");
        if (newsd < 0)
        {
        perror("accept() failed: ");
        return EXIT_FAILURE;
        }
        //use p_thread to handle the client 
        pthread_t thread;
        //set newsd as argument
        pthread_create(&thread, NULL, thread_func, (void *)&newsd);
        pthread_detach(thread);

    }
    

    return EXIT_SUCCESS;
}