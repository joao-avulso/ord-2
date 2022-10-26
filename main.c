#include <stdio.h>
#include <math.h>

#define TAM_MAX_BUCKET 2
#define TAM_MAX_CHAVE 12

typedef struct {
    int prof;
    int cont;
    int chaves[TAM_MAX_BUCKET];
}BUCKET;

typedef struct {
    int bucket_ref;
}DIR_CELL;

void bin(short n){
    unsigned i;
    for (i = 1 << 15; i > 0; i = i / 2)
        (n & i) ? printf("1") : printf("0");
}

short hash (char key[]){
    short sum = 0;
    for (short j = 0; j < TAM_MAX_CHAVE; j++){
        sum = (sum+100*key[j]+key[j+1])%19937;
        j++;
    }
    return sum;
}

short make_address (char key[], int prof){
    short HASH_VAL = hash(key); 
    HASH_VAL = ~HASH_VAL;
    for (short i = prof; i<16; i++){
        HASH_VAL &= ~(1UL << i);
    }
    return HASH_VAL;
}

void inicializa_hash (){
    FILE *bkt = fopen("bucket.dat", "w+b");
    FILE *dir = fopen("dir.dat", "w+b");
    int DIR_PROF, NUM_CELL;

    if (dir == NULL){
        DIR_PROF = 1;
        NUM_CELL = 1;
        DIR_CELL diretorio;
        BUCKET bucket;
        diretorio.bucket_ref = 0;
        bucket.prof = 0;
        fwrite(&diretorio.bucket_ref, sizeof(int), 1, dir);
        fwrite(&bucket.prof, sizeof(int), 1, bkt);
        printf("%d\n", diretorio.bucket_ref);
    }else{
        fseek(dir, 0, SEEK_END);
        DIR_PROF = (int)log2(ftell(dir));
        NUM_CELL = (int)pow(2.0, DIR_PROF);
        DIR_CELL diretorio[NUM_CELL];
        fseek(dir, 0, SEEK_SET);
        for (int i = 0; i < NUM_CELL; i++){
            fread(&diretorio[i].bucket_ref, sizeof(int), 1, dir);
            printf("%d\n", diretorio[i].bucket_ref);
        }
    }
    _fcloseall();
}

int main(){
    //FILE *chv = fopen("chaves-50.txt", "r");

    inicializa_hash();

    /*char chave[TAM_MAX_CHAVE];
    
    fgets(chave, sizeof(chave), chv);
    printf("%s", chave);
    make_address(chave, DIR_PROF);*/

    return 0;
}