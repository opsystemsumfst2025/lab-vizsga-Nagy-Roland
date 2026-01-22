#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>

#define NUM_TRADERS 3
#define BUFFER_SIZE 10
#define INITIAL_BALANCE 10000.0
#define TYPE_LENGTH 5
#define STOCK_LENGTH 5
#define STOCK_NAME_LENGTH 5

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

static StockPrice price_buffer[BUFFER_SIZE];
static size_t buffer_count = 0;
static size_t buffer_read_idx = 0;
static size_t buffer_write_idx = 0;
static double wallet_balance = INITIAL_BALANCE;
static int stocks_owned;

static Transaction *transaction_head = NULL;
static volatile sig_atomic_t running = 1;
static pid_t market_pid = -1;

static const char *stocks[] = {"AAPL", "TSLA", "MSFT", "AMZN", "NVDA", "GOOG"};
static const int num_stocks = (int)(sizeof(stocks)/sizeof(stocks[0]));


// TODO: Implementáld az add_transaction függvényt
// malloc-al foglalj memóriát, töltsd ki a mezőket
// mutex lock alatt add hozzá a láncolt lista elejéhez
void add_transaction(const char *type, const char *stock, int quantity, double price){
    Transaction *tx = malloc(sizeof(Transaction));
    if(tx == NULL){
        return;
    }
    strncpy(tx->type,type,sizeof(tx->type)-1);
    tx->type[sizeof(tx->type)-1] = '\0';

    strncpy(tx->stock,stock,sizeof(tx->stock)-1);
    tx->stock[sizeof(tx->stock)-1] = '\0';

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

void print_transactions(void){
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

void free_transactions(void){
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
        double price = 10.0 + (rand() % 49001) / 100.0;
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

static void buffer_push(const StockPrice *sp) {
    price_buffer[buffer_write_idx] = *sp;
    buffer_write_idx = (buffer_write_idx + 1) % BUFFER_SIZE;
    buffer_count++;
}

static int buffer_pop(StockPrice *out) {
    if (buffer_count == 0) return 0;
    *out = price_buffer[buffer_read_idx];
    buffer_read_idx = (buffer_read_idx + 1) % BUFFER_SIZE;
    buffer_count--;
    return 1;
}


// TODO: Kereskedő szál függvénye
// Végtelen ciklusban:
// - pthread_cond_wait amíg buffer_count == 0
// - Kivesz egy árfolyamot a bufferből (mutex alatt!)
// - Kereskedési döntés (random vagy stratégia)
// - wallet_balance módosítása (MUTEX!!!)
// - add_transaction hívás

static void *trader_thread(void *arg) {
    int id = *(int *)arg;
    free(arg);

    unsigned seed = (unsigned)time(NULL) ^ (unsigned)(uintptr_t)pthread_self();

    while (1) {
        StockPrice sp;

        pthread_mutex_lock(&buffer_mtx);
        while (buffer_count == 0 && running) {
            pthread_cond_wait(&buffer_cond, &buffer_mtx);
        }

        if (!running && buffer_count == 0) {
            pthread_mutex_unlock(&buffer_mtx);
            break;
        }

        (void)buffer_pop(&sp);
        pthread_mutex_unlock(&buffer_mtx);

        /* Döntés: 0=HOLD, 1=BUY, 2=SELL */
        int decision = rand_r(&seed) % 3;
        int qty = 1 + (rand_r(&seed) % 5);

        if (decision == 1) { // BUY
            pthread_mutex_lock(&wallet_mtx);
            double cost = qty * sp.price;
            if (wallet_balance >= cost) {
                wallet_balance -= cost;
                stocks_owned += qty;
                pthread_mutex_unlock(&wallet_mtx);

                add_transaction("BUY", sp.stock_nev, qty, sp.price);
                printf("[Trader %d] BUY  %s qty=%d price=%.2f | balance=%.2f owned=%d\n",
                       id, sp.stock_nev, qty, sp.price, wallet_balance, stocks_owned);
            } else {
                pthread_mutex_unlock(&wallet_mtx);
            }
        } else if (decision == 2) { // SELL
            pthread_mutex_lock(&wallet_mtx);
            if (stocks_owned >= qty) {
                wallet_balance += qty * sp.price;
                stocks_owned -= qty;
                pthread_mutex_unlock(&wallet_mtx);

                add_transaction("SELL", sp.stock_nev, qty, sp.price);
                printf("[Trader %d] SELL %s qty=%d price=%.2f | balance=%.2f owned=%d\n",
                       id, sp.stock_nev, qty, sp.price, wallet_balance, stocks_owned);
            } else {
                pthread_mutex_unlock(&wallet_mtx);
            }
        } else {
            // HOLD (nem csinálunk semmit, mert az emberek ezt is “stratégiának” hívják)
        }
    }
return NULL;
}

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
    signal(SIGINT, sigint_handler);
    
    // TODO: Pipe létrehozása
    // pipe(pipe_fd);
     if (pipe(pipe_fd) != 0) {
        return 1;
    }
    
    // TODO: Fork - Piac folyamat indítása
    // market_pid = fork();
    // Ha gyerek (== 0): piac folyamat
    // Ha szülő: kereskedő szálak indítása

    market_pid = fork();
    if (market_pid < 0) {
        return 1;
    }

     if (market_pid == 0) {
        close(pipe_fd[0]); 
        market_process(pipe_fd[1]);
        _exit(0);
    }

    close(pipe_fd[1]);
    
    // TODO: Kereskedő szálak indítása (pthread_create)
    // for ciklus, malloc az id-nak

     for (int i = 0; i < NUM_TRADERS; i++) {
        int *id = (int *)malloc(sizeof(int));
        if (!id) exit(1);
        *id = i + 1;
        if (pthread_create(&traders[i], NULL, trader_thread, id) != 0) {
            exit(1);
        }
    }
    
    // TODO: Master ciklus
    // Olvasd a pipe-ot (read)
    // Parse-old az árakat
    // Tedd be a bufferbe (mutex alatt!)
    // pthread_cond_broadcast
    
    char buf[256];
    char line[256];
    size_t line_len = 0;

    while (running) {
        ssize_t r = read(pipe_fd[0], buf, sizeof(buf));
        if (r == 0) break;          // EOF
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("read");
            break;
        }

        for (ssize_t i = 0; i < r; i++) {
            char c = buf[i];
            if (c == '\n') {
                line[line_len] = '\0';

                StockPrice sp;
                char stock[STOCK_NAME_LENGTH];
                double price;

                if (sscanf(line, "%4s %lf", stock, &price) == 2) {
                    strncpy(sp.stock_nev, stock, sizeof(sp.stock_nev) - 1);
                    sp.stock_nev[sizeof(sp.stock_nev) - 1] = '\0';
                    sp.price = price;

                    pthread_mutex_lock(&buffer_mtx);
                    if (buffer_count < BUFFER_SIZE) {
                        buffer_push(&sp);
                        pthread_cond_broadcast(&buffer_cond);
                    }
                    pthread_mutex_unlock(&buffer_mtx);
                }

                line_len = 0;
            } else {
                if (line_len + 1 < sizeof(line)) {
                    line[line_len++] = c;
                }
            }
        }
    }
    // TODO: Cleanup
    // pthread_join a szálakra
    // waitpid a Piac folyamatra
    // Végső kiírások
    // free_transactions()
    // mutex destroy
    running = 0;
    pthread_cond_broadcast(&buffer_cond);

    for (int i = 0; i < NUM_TRADERS; i++) {
        pthread_join(traders[i], NULL);
    }

    close(pipe_fd[0]);

    if (market_pid > 0) {
        int status = 0;
        waitpid(market_pid, &status, 0);
    }

    printf("\n=== VEGSO ALLAPOT ===\n");
    printf("Egyenleg: %.2f $\n", wallet_balance);
    printf("Reszvenyek: %d db\n", stocks_owned);
    print_transactions();
    free_transactions();

    pthread_mutex_destroy(&wallet_mtx);
    pthread_mutex_destroy(&buffer_mtx);
    pthread_mutex_destroy(&tx_mtx);
    pthread_cond_destroy(&buffer_cond);

    
    printf("\n[RENDSZER] Sikeres leallitas.\n");
    return 0;
}

