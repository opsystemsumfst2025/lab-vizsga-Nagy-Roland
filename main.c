#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_TRADERS 3
#define BUFFER_SIZE 10
#define INITIAL_BALANCE 10000.0
#define TYPE_LENGTH 4
#define STOCK_LENGTH 4
#define STOCK_NAME_LENGTH 4

pthread_mutex_t wallet_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tx_mtx     = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  buffer_cond = PTHREAD_COND_INITIALIZER;
// TODO: Definiáld a Transaction struktúrát (láncolt lista)
// Tartalmazzon: type, stock, quantity, price, next pointer

typedef struct Transaction{
char type[TYPE_LENGTH];
char stock[STOCK_LENGTH];
int quantity;
double price;
struct Transaction *next;

}Transaction;


// TODO: Definiáld a StockPrice struktúrát
// Tartalmazzon: stock név, price

typedef struct StockPrice{
char stock_nev[STOCK_NAME_LENGTH];
double price;

}StockPrice;

// TODO: Globális változók
// - price_buffer tömb
// - buffer_count, buffer_read_idx, buffer_write_idx
// - wallet_balance, stocks_owned
// - mutex-ek (wallet, buffer, transaction)
// - condition variable
// - transaction_head pointer
// - running flag (volatile sig_atomic_t)
// - market_pid

double price_buffer[BUFFER_SIZE];
size_t buffer_count = 0;
size_t buffer_read_idx = 0;
size_t buffer_write_idx = 0;
long wallet_balance = INITIAL_BALANCE;
long stocks_owned;
Transaction *transaction_head = NULL;
static volatile sig_atomic_t running = 1;
pid_t market_pid = -1;



// TODO: Implementáld az add_transaction függvényt
// malloc-al foglalj memóriát, töltsd ki a mezőket
// mutex lock alatt add hozzá a láncolt lista elejéhez
void add_transaction(const char *type, const char *stock, int quantity, double price){
    Transaction tx* = malloc(sizeof(Transaction));
    if(tx == NULL){
        return;
    }
    strncpy(tx->type,type,sizeof(type));
    tx->type[sizeof(tz->type)-1] = "0";

    strncpy(tx->stock,stock,sizeof(type));
    tx->stock[sizeof(tz->stock)-1] = "0";

    tx-> quantity = quantity;
    tx-> price = price;

    pthread_mutex_lock(&tx_mtx);
    tx->next = transaction_head;
    transaction_head = tx;
    pthread_mutex_unlock(&tx_mtx);
    
}

// TODO: Implementáld a print_transactions függvényt
// Járd végig a láncolt listát mutex lock alatt
// Írd ki az összes tranzakciót

void print_transaction(void){
      pthread_mutex_lock(&tx_mtx);

    Transaction *cur = transaction_head;
    if (cur == NULL) {
        printf("Nincs tranzakcio.\n");
        pthread_mutex_unlock(&tx_mtx);
        return;
    }

    printf("Tranzakciok:\n");
    while (cur != NULL) {
        printf("  %s %s qty=%d price=%.2f\n",
               cur->type, cur->stock, cur->quantity, cur->price);
        cur = cur->next;
    }

    pthread_mutex_unlock(&tx_mtx);
}


// TODO: Implementáld a free_transactions függvényt
// FONTOS: Járd végig a listát és free()-zd az összes elemet
// Ez kell a Valgrind tiszta kimenethez!

void print_transaction(void){
    pthread_mutex_lock(&tx_mtx);

    Transaction *cur = transaction_head;
    transaction_head = NULL;

    pthread_mutex_unlock(&tx_mtx);
    
    while (cur != NULL) {
        Transaction *next = cur-> next;
        free(cur);
        cur = next;
    }   
}

// TODO: Signal handler (SIGINT)
// Állítsd be a running flag-et 0-ra
// Küldj SIGTERM-et a market_pid folyamatnak (kill függvény)
// Ébreszd fel a szálakat (pthread_cond_broadcast)

static void sigint_handler(int sig){

    (void)sig;
    running = 0;

    if (market_pid > 0) {
        kill(market_pid, SIGTERM);
    }

    pthread_cond_broadcast(&buffer_cond);
}

// TODO: Piac folyamat függvénye
// Végtelen ciklusban:
// - Generálj random részvénynevet és árat
// - Írás a pipe_fd-re (write)
// - sleep(1)
static void market_process(int write_fd){

srand((unsigned)time(NULL) ^ (unsigned)getpid());

    while (1) {
        const char *stock = stocks[rand() % num_stocks];

     char line[64];
        int len = snprintf(line, sizeof(line), "%s %.2f\n", stock, price);
        if (len < 0) {
            _exit(EXIT_SUCCESS);
        }

     ssize_t w = write(write_fd, line, (size_t)len);
        if (w < 0) {
            _exit(EXIT_SUCCESS);
        }

        sleep(1);
        
    }
}


// TODO: Kereskedő szál függvénye
// Végtelen ciklusban:
// - pthread_cond_wait amíg buffer_count == 0
// - Kivesz egy árfolyamot a bufferből (mutex alatt!)
// - Kereskedési döntés (random vagy stratégia)
// - wallet_balance módosítása (MUTEX!!!)
// - add_transaction hívás


int main() {
    int pipe_fd[2];
    pthread_t traders[NUM_TRADERS];
    
    printf("========================================\n");
    printf("  WALL STREET - PARHUZAMOS TOZSDE\n");
    printf("========================================\n");
    printf("Kezdo egyenleg: %.2f $\n", INITIAL_BALANCE);
    printf("Kereskedok szama: %d\n", NUM_TRADERS);
    printf("Ctrl+C a leallitashoz\n");
    printf("========================================\n\n");
    
    // TODO: Signal handler regisztrálása
    // signal(SIGINT, ...);
    
    
    // TODO: Pipe létrehozása
    // pipe(pipe_fd);
    
    
    // TODO: Fork - Piac folyamat indítása
    // market_pid = fork();
    // Ha gyerek (== 0): piac folyamat
    // Ha szülő: kereskedő szálak indítása
    
    
    // TODO: Kereskedő szálak indítása (pthread_create)
    // for ciklus, malloc az id-nak
    
    
    // TODO: Master ciklus
    // Olvasd a pipe-ot (read)
    // Parse-old az árakat
    // Tedd be a bufferbe (mutex alatt!)
    // pthread_cond_broadcast
    
    
    // TODO: Cleanup
    // pthread_join a szálakra
    // waitpid a Piac folyamatra
    // Végső kiírások
    // free_transactions()
    // mutex destroy
    
    
    printf("\n[RENDSZER] Sikeres leallitas.\n");
    return 0;
}
