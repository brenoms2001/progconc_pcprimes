#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<math.h>
#include<unistd.h>
#include<time.h>

//Macros
#define P 1
#define C 5
#define N 300000
#define M 10
#define MAX 10000000

//Variáveis contadoras
int Buffer[N];
int count=0, in=0, out=0, k, primos;

//Variáveis para sincronizacao
pthread_mutex_t mutex;
pthread_cond_t cond_cons, cond_prod;

//Declarações de função
int ehPrimo(long long int n);

//Structs
typedef struct {
   int idThread, nPrimos;
} t_Consumidora;

t_Consumidora t_Vencedora = {-1,-1};

//Cópia a adaptar
//---------------------------------------------------------------------------------------
//inicializa o buffer
void IniciaBuffer() {
  for(int i=0; i<M; i++)
    Buffer[i] = 0;
}

//imprime o buffer
void ImprimeBuffer() {
  for(int i=0; i<M; i++)
    printf("%d ", Buffer[i]);
  printf("\n");
}

//insere um elemento no Buffer ou bloqueia a thread caso o Buffer esteja cheio
void Insere (int item, int id) {
   pthread_mutex_lock(&mutex);
   printf("P[%d] quer inserir\n", id);
   while(count == M) {
     printf("P[%d] bloqueou\n", id);
     pthread_cond_wait(&cond_prod, &mutex);
     printf("P[%d] desbloqueou\n", id);
   }
   Buffer[in] = item;
   in = (in + 1)%M;
   count++;
   printf("P[%d] inseriu\n", id);
   ImprimeBuffer();
   pthread_mutex_unlock(&mutex);
   pthread_cond_signal(&cond_cons);
}

//retira um elemento no Buffer ou bloqueia a thread caso o Buffer esteja vazio
int Retira (int id) {
  int item;
  pthread_mutex_lock(&mutex);
  printf("C[%d] quer consumir\n", id);
  while(count == 0 && k!=N) {
    printf("C[%d] bloqueou\n", id);
    pthread_cond_wait(&cond_cons, &mutex);
    printf("C[%d] desbloqueou\n", id);
  }
  if (count == 0 && k == N) {
   pthread_mutex_unlock(&mutex);
   return -1; // sinaliza que acabou
  } 
  item = Buffer[out];
  Buffer[out] = 0;
  out = (out + 1)%M;
  count--;
  printf("C[%d] consumiu %d\n", id, item);
  ImprimeBuffer();
  pthread_mutex_unlock(&mutex);
  pthread_cond_signal(&cond_prod);
  return item;
}

//thread produtora
void * produtor(void * arg) {
  int *id = (int *) arg;
  printf("Sou a thread produtora %d\n", *id);
  for(k=0; k<N;k++) {
    int item = rand() % MAX;
    Insere(item, *id);
    k++;
    sleep(1/5);
  } 
  // Ao terminar, sinaliza as consumidoras para que saiam se estiverem esperando
  pthread_mutex_lock(&mutex);
  pthread_cond_broadcast(&cond_cons);
  pthread_mutex_unlock(&mutex);
  free(arg);
  pthread_exit(NULL);
}

//thread consumidora
void * consumidor(void * arg) {
  t_Consumidora *id = (t_Consumidora *) arg;
  int item;
  printf("Sou a thread consumidora %d\n", id->idThread);
  while(1) {
    item = Retira(id->idThread);
    if (item == -1) break;
    if (ehPrimo(item)) {
      id->nPrimos++;
      primos++;
      printf("Encontrei o primo %d\n", item);
    }    
    sleep(1);
  }
  pthread_mutex_lock(&mutex);
  if(id->nPrimos>t_Vencedora.nPrimos){
    t_Vencedora.idThread = id->idThread;
    t_Vencedora.nPrimos = id->nPrimos;
  }
  pthread_mutex_unlock(&mutex);
  free(arg);
  pthread_exit(NULL);
}
//------------------------------------------------------------------------------------------

int main(int argc, char* argv[]){
  //variaveis auxiliares
  int i;
 
  //identificadores das threads
  pthread_t tid[P+C];
  int *id[P+C];

  //seed de tempo
  srand(time(NULL));

  //aloca espaco para os IDs das threads
  for(i=0; i<P+C;i++) {
    id[i] = malloc(sizeof(int));
    if(id[i] == NULL) exit(-1);
    *id[i] = i+1;
  }

  //inicializa o Buffer
  IniciaBuffer();  

  //inicializa as variaveis de sincronizacao
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond_cons, NULL);
  pthread_cond_init(&cond_prod, NULL);

  //cria as threads produtoras
  for(i=0; i<P; i++) {
    if(pthread_create(&tid[i], NULL, produtor, (void *) id[i])) exit(-1);
  } 
  
  //cria as threads consumidoras
  for(i=0; i<C; i++) {
    if(pthread_create(&tid[i+P], NULL, consumidor, (void *) id[i+P])) exit(-1);
  } 

  //espera pelo termino da threads
  for(int i=0; i<P+C; i++) {
    pthread_join(*(tid+i), NULL);
  }
  printf("O programa teve um total de: %d primos\n", primos);
  printf("A thread vencedora foi a %d com %d primos identificados\n", t_Vencedora.idThread, t_Vencedora.nPrimos);
  return 0;
}

//Implementações
int ehPrimo(long long int n) {
    int i;
    if (n<=1) return 0;
    if (n==2) return 1;
    if (n%2==0) return 0;
    for (i=3; i<sqrt(n)+1; i+=2)
        if(n%i==0) return 0;
return 1;
}