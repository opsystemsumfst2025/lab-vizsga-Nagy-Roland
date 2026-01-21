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

// TODO: Definiáld a Transaction struktúrát (láncolt lista)
// Tartalmazzon: type, stock, quantity, price, next pointer

typedef struct Transaction{
char type[TYPE_LENGTH];
char stock[STOCK_LENGTH];
int quantiy;
double price;
struct Transaction next*;

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

size_t buffer_count = 0;
size_t buffer_read_idx = 0;
size_t buffer_write_idx = 0;



double price_buffer[BUFFER_SIZE];

// TODO: Implementáld az add_transaction függvényt
// malloc-al foglalj memóriát, töltsd ki a mezőket
// mutex lock alatt add hozzá a láncolt lista elejéhez


// TODO: Implementáld a print_transactions függvényt
// Járd végig a láncolt listát mutex lock alatt
// Írd ki az összes tranzakciót


// TODO: Implementáld a free_transactions függvényt
// FONTOS: Járd végig a listát és free()-zd az összes elemet
// Ez kell a Valgrind tiszta kimenethez!


// TODO: Signal handler (SIGINT)
// Állítsd be a running flag-et 0-ra
// Küldj SIGTERM-et a market_pid folyamatnak (kill függvény)
// Ébreszd fel a szálakat (pthread_cond_broadcast)


// TODO: Piac folyamat függvénye
// Végtelen ciklusban:
// - Generálj random részvénynevet és árat
// - Írás a pipe_fd-re (write)
// - sleep(1)


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
