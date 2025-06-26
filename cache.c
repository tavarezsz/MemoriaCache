#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

typedef struct {
    int tag;
    bool valido; //indica se o bloco foi carregado da memoria
    bool modificado; //dirty
    short ultimoUso; //para o LRU
} Linha;

typedef struct{
    Linha* linhas;
} Conjunto;

typedef struct {
    Conjunto* conjuntos;
    int numConjuntos;
    int tamConjunto;
    int tamBloco;
    int politicaEscrita; //0 - write-through e 1 - write-back
    int politicaSubstituicao; //0 - LRU e 1 - aleatória
    int tempoAcerto;
} Cache;

typedef struct {
    int hitsLeitura;
    int missesLeitura;
    int hitsEscrita;
    int missesEscrita;
    int leiturasMP;
    int escritasMP;
} Variaveis;

int potenciaDeDois(int n) {
    int potencia = 0;

    while (n > 1) {
        n /= 2;
        potencia++;
    }

    return potencia;
}

int encontrarBlocoParaSubstituir(Cache *cache, int indexConjunto){
    int maxUltimoUso = -1;
    int linhaMaxUltimoUso = -1;

    Conjunto *conjunto = &cache->conjuntos[indexConjunto];
    int i;

    for(i = 0; i < cache->tamConjunto; i++){
        if(!conjunto->linhas[i].valido) return i; //se tiver linha vazia, retorna essa linha
        if(conjunto->linhas[i].ultimoUso > maxUltimoUso){
            maxUltimoUso = conjunto->linhas[i].ultimoUso;
            linhaMaxUltimoUso = i;
        }
    }

    if(cache->politicaSubstituicao == 1) { //aleatorio
            return rand() % cache->tamConjunto;
    } else { //LRU
            return linhaMaxUltimoUso;
    }


}


void acesso(Cache *cache, int endereco, char operacao, Variaveis *variaveis){
    
    //quantidade de bits para identificar a palavra e o conjunto
    int bitsPalavra = potenciaDeDois(cache->tamBloco);
    int bitsConjunto = potenciaDeDois(cache-> numConjuntos);

    int mascaraConjunto = (1 << bitsConjunto) - 1;  //gera um valor com n 1s no final, sendo n o numero de bits da palavra
    int indexConjunto = (endereco >> bitsPalavra) & mascaraConjunto; //and para deixar só os bits do conjunto

    //a tag é o que sobra
    int tag = endereco >> (bitsPalavra + bitsConjunto);


    Conjunto *conjunto = &cache->conjuntos[indexConjunto];
    int i;
    int linhaConjunto = -1;

    for(i = 0; i < cache->tamConjunto; i++){
        Linha *linha = &conjunto->linhas[i];

        if (linha->valido) {
            linha->ultimoUso++;
        }

        if (linha->valido && linha->tag == tag && linhaConjunto == -1) {
            linhaConjunto = i;
        }
    }

    if(linhaConjunto != -1){
        //hit
        Linha *linha = &conjunto->linhas[linhaConjunto];
        linha->ultimoUso = 0;

        if (operacao == 'R') {
            variaveis->hitsLeitura++;
        } else {
            variaveis->hitsEscrita++;

            if (cache->politicaEscrita == 0) {
                variaveis->escritasMP++;
            } else {
                linha->modificado = true;
            }
        }
           
    } else {
        //miss
       if (operacao == 'R') {
            variaveis->missesLeitura++;
            variaveis->leiturasMP++;
        } else {
            variaveis->missesEscrita++;
                    
            if (cache->politicaEscrita == 0) {
                variaveis->escritasMP++;
                return; // write-trough usa no-write-allocate e não altera a cache, escreve só na mp
            }

            variaveis->leiturasMP++;
        }

        int indiceLinhaASerSubstituida = encontrarBlocoParaSubstituir(cache, indexConjunto);
        Linha *linha = &conjunto->linhas[indiceLinhaASerSubstituida];


        //write-back -> escreve na memória principal antes de substituir
        if (cache->politicaEscrita == 1 && linha->valido && linha->modificado) {
            variaveis->escritasMP++;
        }

        linha->tag = tag;
        linha->valido = true;
        linha->modificado = (operacao == 'W' && cache->politicaEscrita == 1);
        linha->ultimoUso = 0;

    }
}


Cache* inicializarCache(int tamBloco, int numLinhas, int associatividade, 
    int politicaEscrita, int politicaSubstituicao, int tempoAcerto) {
    Cache *cache = (Cache*)malloc(sizeof(Cache));
    
    cache->numConjuntos = numLinhas / associatividade;
    cache->tamConjunto = associatividade;
    cache->tamBloco = tamBloco;
    cache->politicaEscrita = politicaEscrita;
    cache->politicaSubstituicao = politicaSubstituicao;
    cache->tempoAcerto = tempoAcerto;
    
    int i;
    cache->conjuntos = (Conjunto*)malloc(cache->numConjuntos * sizeof(Conjunto));
    for (i = 0; i < cache->numConjuntos; i++) {
        cache->conjuntos[i].linhas = (Linha*)malloc(cache->tamConjunto * sizeof(Linha));
        for (int j = 0; j < cache->tamConjunto; j++) {
            cache->conjuntos[i].linhas[j].valido = false;
            cache->conjuntos[i].linhas[j].modificado = false;
            cache->conjuntos[i].linhas[j].tag = 0;
            cache->conjuntos[i].linhas[j].ultimoUso = 0;
        
        }
    }
    
    return cache;
}


void liberarCache(Cache *cache) {
    int i;
    for (i = 0; i < cache->numConjuntos; i++) {
        free(cache->conjuntos[i].linhas);
    }
    free(cache->conjuntos);
    free(cache);
}

void atualizarMP(Cache *cache, Variaveis *vars){
    int i, j;
    for(i = 0; i < cache->numConjuntos; i++){
        for(j = 0; j < cache->tamConjunto; j++){
            Linha linha = cache->conjuntos[i].linhas[j];
            if(linha.valido && linha.modificado) vars->escritasMP++;
        }
    }
}


int main(){

    srand(time(NULL));
    int escrita, tamLinha, numLinhas, associatividade, tempoAcerto, substituicao;
 
    char arquivoEntrada[100];
    char arquivoSaida[100];

    //teste.cache o u oficial.cache
    printf("Digite o nome do arquivo de entrada: ");
    scanf("%s", arquivoEntrada);

    printf("Digite o nome do arquivo de saída: ");
    scanf("%s", arquivoSaida);

    printf("Tamanho do bloco (bytes): ");
    scanf("%d", &tamLinha);

    printf("Número de linhas da cache: ");
    scanf("%d", &numLinhas);

    printf("Associatividade (linhas por conjunto): ");
    scanf("%d", &associatividade);

    printf("Hit time (ns): ");
    scanf("%d", &tempoAcerto);
   
    printf("Política de escrita (0 - write-through, 1 - write-back): ");
    scanf("%d", &escrita);

    printf("Política de substituição (0 - LRU, 1 - Aleatório): ");
    scanf("%d", &substituicao);

    Cache *cache = inicializarCache(tamLinha, numLinhas, associatividade, escrita, substituicao, tempoAcerto);
    Variaveis vars = {0};

    
    int tempoMP = 70;

    printf("Tempo de leitura/escrita da memória principal (ns): ");
    scanf("%d", &tempoMP);

    FILE* f = fopen(arquivoEntrada, "r");
    if (f == NULL) {
        printf("Erro ao abrir arquivo de entrada.\n");
        return 0;
    }

    //deixar o endereco direto em int pra manipular o endereco usando operacoes bitwise
    int endereco; char operacao;

    //%x scaneia hexa
    while (fscanf(f, "%x %c", &endereco, &operacao) != EOF) {
         acesso(cache, endereco, operacao, &vars);
    }

    fclose(f);

    //atualizar a MP após o término da simulação
    if(cache->politicaEscrita == 1){
        atualizarMP(cache, &vars);
    }

    int totalLeituras = vars.hitsLeitura + vars.missesLeitura;
    int totalEscritas = vars.hitsEscrita + vars.missesEscrita;

    int totalAcessos = totalLeituras + totalEscritas;

    int totalHits = vars.hitsLeitura + vars.hitsEscrita;

    float taxaHitEscrita = (float)vars.hitsEscrita / totalEscritas;
    float taxaHitLeitura = (float)vars.hitsLeitura / totalLeituras;
    float taxaHitGlobal =  (float)totalHits / totalAcessos;

    float tempoMedioAcesso = (float)cache->tempoAcerto + (1 - taxaHitGlobal) * tempoMP;
    
    FILE* saida = fopen(arquivoSaida, "w");
    if (!saida) {
        printf("Erro ao criar arquivo de saída.\n");
        liberarCache(cache);
        return 0;
    }

    fprintf(saida, "=== Parâmetros ===\n");
    fprintf(saida, "Arquivo de entrada: %s\n", arquivoEntrada);
    fprintf(saida, "Tamanho da linha: %d bytes\n", tamLinha);
    fprintf(saida, "Número de linhas: %d\n", numLinhas);
    fprintf(saida, "Associatividade: %d\n", associatividade);
    fprintf(saida, "Tempo de hit: %d ns\n", tempoAcerto);
    fprintf(saida, "Política de escrita: %s\n", escrita ? "write-back" : "write-through");
    fprintf(saida, "Política de substituição: %s\n", substituicao ? "Aleatória" : "LRU");

    fprintf(saida, "\n=== Resultados ===\n");
    fprintf(saida, "Total de acessos à cache: %d\n", totalAcessos);
    fprintf(saida, " - Leituras: %d\n", totalLeituras);
    fprintf(saida, " - Escritas: %d\n", totalEscritas);
    fprintf(saida, "\nTotal de acessos à memória principal:\n");
    fprintf(saida, " - Leituras: %d\n", vars.leiturasMP);
    fprintf(saida, " - Escritas: %d\n", vars.escritasMP);
    fprintf(saida, "\nTaxas de acerto:\n");
    fprintf(saida, "  Leituras: %.4f (%d/%d)\n", taxaHitLeitura, vars.hitsLeitura, totalLeituras);
    fprintf(saida, "  Escritas: %.4f (%d/%d)\n", taxaHitEscrita, vars.hitsEscrita, totalEscritas);
    fprintf(saida, "  Global: %.4f (%d/%d)\n", taxaHitGlobal, totalHits, totalAcessos);
    fprintf(saida, "\nTempo médio de acesso: %.4f ns\n", tempoMedioAcesso);
    
    fclose(saida);
    liberarCache(cache);

    return 0;
}

