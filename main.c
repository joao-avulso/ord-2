#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define TAM_MAX_BUCKET 5
#define SUCCESS true
#define FAILURE false

int DIR_PROF;

typedef struct {
    int prof;
    int cont;
    int chaves[TAM_MAX_BUCKET];
}BUCKET;

typedef struct {
    int bucket_ref;
}DIR_CELL;

int make_address(int key, int prof){
    int RETVAL = 0;
    int MASK = 1;
    int LOWBIT;
    for(int j=0;j<prof;j++){
        RETVAL = (RETVAL << 1);
        LOWBIT = key & MASK;
        RETVAL = (RETVAL | LOWBIT);
        key = (key >> 1);
    }
    return RETVAL;
}

void atualiza_dir(DIR_CELL *diretorio){
    FILE *dir = fopen("dir.dat", "wb");
    for(int i=0; i<pow(2,DIR_PROF);i++){
        fwrite(&diretorio[i].bucket_ref, sizeof(int), 1, dir);
    }
    fclose(dir);
}

void find_new_range(BUCKET *OLD_BUCKET, int *NEW_START, int *NEW_END){
    int SHARED_ACCESS = make_address(OLD_BUCKET->chaves[0], OLD_BUCKET->prof);
    SHARED_ACCESS = (SHARED_ACCESS << 1);
    SHARED_ACCESS = (SHARED_ACCESS | 1);

    int BITS_TO_FILL = DIR_PROF - (OLD_BUCKET->prof + 1);
    
    *NEW_START = SHARED_ACCESS;
    *NEW_END = SHARED_ACCESS;
    for (int i = 1; i <= BITS_TO_FILL; i++){
        *NEW_START = (*NEW_START << 1);
        *NEW_END = (*NEW_END << 1);
        *NEW_END = (*NEW_END | 1);
    }
}

void dir_ins_bucket(int BUCKET_ADDRESS, int START, int END, DIR_CELL *diretorio){
    for(int j = START; j <= END; j++){
        diretorio[j].bucket_ref = BUCKET_ADDRESS;
    }
}

void dir_double(DIR_CELL *OLD_DIR){
    int TAM_ATUAL = (pow(2, DIR_PROF));
    int NOVO_TAM = TAM_ATUAL * 2;
    DIR_CELL *NOVO_DIR = calloc(NOVO_TAM, sizeof(DIR_CELL));
    for(int i=0; i<TAM_ATUAL; i++){
        NOVO_DIR[2*i].bucket_ref = OLD_DIR[i].bucket_ref;
        NOVO_DIR[(2*i)+1].bucket_ref = OLD_DIR[i].bucket_ref;
    }
    OLD_DIR = realloc(OLD_DIR, sizeof(DIR_CELL));
    for(int i=0; i<NOVO_TAM; i++)
        OLD_DIR[i].bucket_ref = NOVO_DIR[i].bucket_ref;
    DIR_PROF++;
}

void bk_split(BUCKET *OLD_BUCKET, DIR_CELL *d, int rrn_bkt){
    if(OLD_BUCKET->prof == DIR_PROF){
        dir_double(d);
    }
    BUCKET *NOVO_BUCKET = calloc(1, sizeof(BUCKET));
    FILE *bkt = fopen("bucket.dat", "rb");
    fseek(bkt, 0, SEEK_END);
    int END_NOVO_BUCKET = ftell(bkt);
    int NEW_START, NEW_END;
    find_new_range(OLD_BUCKET, &NEW_START, &NEW_END);
    dir_ins_bucket(END_NOVO_BUCKET, NEW_START, NEW_END, d);
    OLD_BUCKET->prof++;
    NOVO_BUCKET->prof = OLD_BUCKET->prof;
    int j=0;
    for(int i=(int)(TAM_MAX_BUCKET/2)-1; i<TAM_MAX_BUCKET; i++){
        NOVO_BUCKET->chaves[j] = OLD_BUCKET->chaves[i];
        OLD_BUCKET->chaves[i] = 0;
        OLD_BUCKET->cont--;
        NOVO_BUCKET->cont++;
        j++;
    }
    fclose(bkt);
    bkt = fopen("bucket.dat", "r+b");
    fseek(bkt, rrn_bkt, SEEK_SET);
    fwrite(OLD_BUCKET, sizeof(BUCKET), 1, bkt);
    fseek(bkt, END_NOVO_BUCKET, SEEK_SET);
    fwrite(NOVO_BUCKET, sizeof(BUCKET), 1, bkt);
    atualiza_dir(d);
    fclose(bkt);
}

bool op_find(int key, int *FOUND_BUCKET, DIR_CELL *d){
    short address = make_address(key, DIR_PROF);
    *FOUND_BUCKET = d[address].bucket_ref;

    FILE *bkt = fopen("bucket.dat", "r+b");
    fseek(bkt, *FOUND_BUCKET, SEEK_SET);
    BUCKET *b = calloc(1, sizeof(BUCKET));
    fread(b, sizeof(BUCKET), 1, bkt);

    for(int i=0;i<TAM_MAX_BUCKET;i++){
        if(b->chaves[i] == key){
            free(b);
            fclose(bkt);
            return SUCCESS;
        }
    }
    free(b);
    fclose(bkt);
    return FAILURE;
}

bool op_add(int key, DIR_CELL *d){
    int FOUND_BUCKET;
    atualiza_dir(d);
    if(op_find(key, &FOUND_BUCKET, d) == SUCCESS)
        return FAILURE;
    //bk_add_key
    FILE *bkt = fopen("bucket.dat", "r+b");
    fseek(bkt, FOUND_BUCKET, SEEK_SET);
    BUCKET *b = calloc(1, sizeof(BUCKET));
    fread(b, sizeof(BUCKET), 1, bkt);

    if(b->cont < TAM_MAX_BUCKET){
        b->chaves[b->cont] = key;
        b->cont++;
        fseek(bkt, FOUND_BUCKET, SEEK_SET);
        fwrite(b, sizeof(BUCKET), 1, bkt);
        fclose(bkt);
    }
    else{
        bk_split(b, d, FOUND_BUCKET);
        op_add(key, d);
    }
    return SUCCESS;
}

DIR_CELL *inicializa_dir(){
    FILE *dir = fopen("dir.dat", "r+b");

    if (dir == NULL){
        dir = fopen("dir.dat", "w+b");
        FILE *bkt = fopen("bucket.dat", "wb");
        BUCKET *bucket = calloc(1, sizeof(BUCKET));
        DIR_CELL *diretorio = calloc(1, sizeof(DIR_CELL));
        diretorio[0].bucket_ref = 0;
        fwrite(&diretorio[0].bucket_ref, sizeof(int), 1, dir);
        fwrite(&bucket[0], sizeof(BUCKET), 1, bkt);
        fclose(bkt);
        return diretorio;
    }else{
        fseek(dir, 0, SEEK_END);
        int NUM_CELLS = ftell(dir) / sizeof(int);
        DIR_CELL *diretorio = calloc(1, sizeof(DIR_CELL)*(NUM_CELLS));
        DIR_PROF = (int)log2(NUM_CELLS);
        rewind(dir);
        for (int i = 0; i < NUM_CELLS; i++){
            fread(&diretorio[i].bucket_ref, sizeof(int), 1, dir);
        }
        return diretorio;
    }
}

void imprime_dir(){
    DIR_CELL *dir = inicializa_dir();
    FILE *bkt = fopen("bucket.dat", "rb");
    fseek(bkt, 0, SEEK_END);
    printf("---- Diretorio ----\n");
    for(int i=0; i<(pow(2, DIR_PROF)); i++)
        printf("dir[%d] = bucket(%d)\n", i, (dir[i].bucket_ref/sizeof(BUCKET)));
    printf("\nProfundidade = %d", DIR_PROF);
    printf("\nTamanho Atual = %d", (int)(pow(2, DIR_PROF)));
    printf("\nTotal de Buckets = %d", (int)(ftell(bkt)/sizeof(BUCKET)));
    printf("\n\n");
    fclose(bkt);
}

void imprime_bkt(){
    FILE *bkt = fopen("bucket.dat", "rb");
    fseek(bkt, 0, SEEK_END);
    int END = ftell(bkt);
    rewind(bkt);
    BUCKET *b = calloc(1, sizeof(BUCKET));
    printf("---- Buckets ----\n");

    do{
        fread(b, sizeof(BUCKET), 1, bkt);
        printf("Bucket %d (Prof = %d):\n", (int)(ftell(bkt)/sizeof(BUCKET))-1, b->prof);
        for(int i=0; i<b->cont; i++)
            printf("Chave[%d] = %d\n", i, b->chaves[i]);
        printf("\n");
    }while(ftell(bkt)<END);
    fclose(bkt);
}

void importa_chaves(char arq_chaves[]){
    DIR_CELL *dir = inicializa_dir();
    FILE *chv = fopen(arq_chaves, "r");
    if(chv == NULL){
        printf("Arquivo de chaves não encontrado");
        exit(EXIT_FAILURE);
    }
        
    char buffer[12];
    int x,y;

    y = fgets(buffer, sizeof(buffer), chv);
    while(y > 0){
        x = atoi(buffer);
        op_add(x, dir);
        y = fgets(buffer, sizeof(buffer), chv);
    }
}

int main(int argc, char *argv[]){
    /*if(argc == 2){
        if(!strcmp(argv[1], "-pd"))
            imprime_dir();
        else if(!strcmp(argv[1], "-pb"))
            imprime_bkt();
    }
    else if(argc == 3 && (!strcmp(argv[1], "-i")))
        importa_chaves(argv[2]);
    else
        printf("\nArgumentos invalidos\n");
    
    return 0;*/
    importa_chaves("chaves-50.txt");
    imprime_dir();
}