#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

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

bool ehPotenciaDeDois(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

int potenciaDeDois(int n) {
    if (!ehPotenciaDeDois(n)) return -1;
    
    int potencia = 0;
    while (n > 1) {
        n /= 2;
        potencia++;
    }
    return potencia;
}

void mostrarCache(Cache *cache) {
    printf("\nEstado final da Cache:\n");
    printf("+---------+----------+-------+-------+-------+\n");
    printf("|Conjunto | Tag      | Válido| Modif.| Uso   |\n");
    printf("+---------+----------+-------+-------+-------+\n");
    
    int i, j;
    for (i = 0; i < cache->numConjuntos; i++) {
        for (j = 0; j < cache->tamConjunto; j++) {
            Linha linha = cache->conjuntos[i].linhas[j];
            printf("|%-9d| %-10x| %-7d| %-7d| %-6d|\n", 
                   i, linha.tag, linha.valido, linha.modificado, linha.ultimoUso);
        }
        printf("+---------+----------+-------+-------+-------+\n");
    }
}

int encontrarBlocoParaSubstituir(Cache *cache, int indexConjunto){
    int maxUltimoUso = -1;
    int linhaMaxUltimoUso = -1;

    Conjunto *conjunto = &cache->conjuntos[indexConjunto];
    int i;

    for(i = 0; i < cache->tamConjunto; i++){
        if(!conjunto->linhas[i].valido) {
            return i;
        }
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
    int bitsConjunto = potenciaDeDois(cache->numConjuntos);

    int mascaraConjunto = (1 << bitsConjunto) - 1;
    int indexConjunto = (endereco >> bitsPalavra) & mascaraConjunto;

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
    
    int i, j;
    cache->conjuntos = (Conjunto*)malloc(cache->numConjuntos * sizeof(Conjunto));
    for (i = 0; i < cache->numConjuntos; i++) {
        cache->conjuntos[i].linhas = (Linha*)malloc(cache->tamConjunto * sizeof(Linha));
        for (j = 0; j < cache->tamConjunto; j++) {
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
            if(linha.valido && linha.modificado) {
                vars->escritasMP++;
            }
        }
    }
}

int main(){
    srand(time(NULL));
    int escrita, tamLinha, numLinhas, associatividade, tempoAcerto, substituicao;
    char arquivoEntrada[200];
    char arquivoSaida[100];
    int tempoMP = 70;

    printf("Digite o nome do arquivo de entrada: ");
    fgets(arquivoEntrada, sizeof(arquivoEntrada), stdin);
    arquivoEntrada[strcspn(arquivoEntrada, "\n")] = 0; 

    printf("Digite o nome do arquivo de saída: ");
    fgets(arquivoSaida, sizeof(arquivoSaida), stdin);
    arquivoSaida[strcspn(arquivoSaida, "\n")] = 0; 


    printf("Tamanho do bloco (bytes, potência de 2): ");
    scanf("%d", &tamLinha);

    printf("Número de linhas da cache (potência de 2): ");
    scanf("%d", &numLinhas);

    printf("Associatividade (linhas por conjunto, deve dividir o número de linhas): ");
    scanf("%d", &associatividade);

    printf("Hit time (ns): ");
    scanf("%d", &tempoAcerto);
   
    printf("Política de escrita (0 - write-through, 1 - write-back): ");
    scanf("%d", &escrita);

    printf("Política de substituição (0 - LRU, 1 - Aleatório): ");
    scanf("%d", &substituicao);

    printf("Tempo de leitura/escrita da memória principal (ns): ");
    scanf("%d", &tempoMP);

    if (!ehPotenciaDeDois(tamLinha)) {
        printf("Erro: O tamanho do bloco deve ser potência de 2.\n");
        return 1;
    }

    if (!ehPotenciaDeDois(numLinhas)) {
        printf("Erro: O número de linhas deve ser potência de 2.\n");
        return 1;
    }

    if (numLinhas % associatividade != 0) {
        printf("Erro: O número de linhas deve ser divisível pela associatividade.\n");
        return 1;
    }

    if (escrita < 0 || escrita > 1) {
        printf("Erro: Política de escrita deve ser 0 (write-through) ou 1 (write-back).\n");
        return 1;
    }

    if (substituicao < 0 || substituicao > 1) {
        printf("Erro: Política de substituição deve ser 0 (LRU) ou 1 (Aleatória).\n");
        return 1;
    }

    if (tempoMP <= tempoAcerto) {
        printf("Aviso: O tempo de acesso à MP normalmente é maior que o tempo de hit da cache.\n");
    }

    Cache *cache = inicializarCache(tamLinha, numLinhas, associatividade, escrita, substituicao, tempoAcerto);
    Variaveis vars = {0};

    FILE* f = fopen(arquivoEntrada, "r");
    if (f == NULL) {
        perror("Erro ao abrir arquivo de entrada");
        liberarCache(cache);
        return 1;
    }

    int endereco; 
    char operacao;
    while (fscanf(f, "%x %c", &endereco, &operacao) != EOF) {
        acesso(cache, endereco, operacao, &vars);
    }

    fclose(f);

    // Atualizar a MP após o término da simulação (para write-back)
    if(cache->politicaEscrita == 1){
        atualizarMP(cache, &vars);
    }

    int totalLeituras = vars.hitsLeitura + vars.missesLeitura;
    int totalEscritas = vars.hitsEscrita + vars.missesEscrita;
    int totalAcessos = totalLeituras + totalEscritas;
    int totalHits = vars.hitsLeitura + vars.hitsEscrita;

    float taxaHitEscrita = totalEscritas > 0 ? (float)vars.hitsEscrita / totalEscritas : 0;
    float taxaHitLeitura = totalLeituras > 0 ? (float)vars.hitsLeitura / totalLeituras : 0;
    float taxaHitGlobal = totalAcessos > 0 ? (float)totalHits / totalAcessos : 0;

    float tempoMedioAcesso = cache->tempoAcerto + (1 - taxaHitGlobal) * tempoMP;
    
    //mostrar estado final da cache
    mostrarCache(cache);

    FILE* saida = fopen(arquivoSaida, "w");
    if (!saida) {
        printf("Erro ao criar arquivo de saída.\n");
        liberarCache(cache);
        return 1;
    }

    fprintf(saida, "=== Parâmetros ===\n");
    fprintf(saida, "Arquivo de entrada: %s\n", arquivoEntrada);
    fprintf(saida, "Tamanho da linha: %d bytes\n", tamLinha);
    fprintf(saida, "Número de linhas: %d\n", numLinhas);
    fprintf(saida, "Associatividade: %d\n", associatividade);
    fprintf(saida, "Tempo de hit: %d ns\n", tempoAcerto);
    fprintf(saida, "Política de escrita: %s\n", escrita ? "write-back" : "write-through");
    fprintf(saida, "Política de substituição: %s\n", substituicao ? "Aleatória" : "LRU");
    fprintf(saida, "Tempo de acesso à MP: %d ns\n", tempoMP);

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

    printf("\nSimulação concluída. Resultados salvos em %s\n", arquivoSaida);

    return 0;
}