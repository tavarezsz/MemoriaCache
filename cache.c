#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int tag;
    int valido; //indica se o bloco foi carregado da memoria, para evitar ler lixo no inicio do programa quando a cache 
                //ainda não foi totalmente preenchida
    int modificado; //dirty
    int ultimoUso; //para o LRU
} Linha;

typedef struct{
    Linha* linhas;
} Conjunto;

typedef struct {
    Conjunto* conjuntos;
    int numConjuntos;
    int tamConjunto;
    int tamBloco;
    //deixar esses campos na cache pra n ter q ficar passando como parametro em tudo
    int politicaEscrita; //0 - write-through e 1 - write-back
    int politicaSubstituicao; //0 - LRU e 1 - aleatória
    int tempoAcerto;
} Cache;

//tem que ser configurável pelo usuário, mesmo q pros testes seja sempre 70ns
typedef struct {
    int tempoEscrita;
    int tempoLeitura;
} MemoriaPrincipal;

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

    int mascaraConjunto = (1 << bitsConjunto) - 1;  //cria um valor com n 1s no final, sendo n o numero de bits da palavra
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
                linha->modificado = 1;
            }
        }
           
    } else {
        //miss
       if (operacao == 'R') {
            variaveis->missesLeitura++;
            variaveis->leiturasMP++;
        } else {
            variaveis->missesEscrita++;
            variaveis->leiturasMP++;
            
            if (cache->politicaEscrita == 0) {
                variaveis->escritasMP++;
            }
        }

        int indiceLinhaASerSubstituida = encontrarBlocoParaSubstituir(cache, indexConjunto);
        Linha *linha = &conjunto->linhas[indiceLinhaASerSubstituida];


        //write-back -> escreve na memória principal antes de substituir
        if (cache->politicaEscrita == 1 && linha->valido && linha->modificado) {
            variaveis->escritasMP++;
        }

        linha->tag = tag;
        linha->valido = 1;
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
    
    cache->conjuntos = (Conjunto*)malloc(cache->numConjuntos * sizeof(Conjunto));
    for (int i = 0; i < cache->numConjuntos; i++) {
        cache->conjuntos[i].linhas = (Linha*)malloc(cache->tamConjunto * sizeof(Linha));
        for (int j = 0; j < cache->tamConjunto; j++) {
            cache->conjuntos[i].linhas[j].valido = 0;
            cache->conjuntos[i].linhas[j].modificado = 0;
            cache->conjuntos[i].linhas[j].tag = 0;
            cache->conjuntos[i].linhas[j].ultimoUso = 0;
        
        }
    }
    
    return cache;
}


void liberarCache(Cache *cache) {
    for (int i = 0; i < cache->numConjuntos; i++) {
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

    //por enquanto vai ser o de teste
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

    //printf("Hit time (ns): "); //sempre 5 ns para os testes
    //scanf("%d", &tempoAcerto);
    tempoAcerto = 5;

    printf("Política de escrita (0 - write-through, 1 - write-back): ");
    scanf("%d", &escrita);

    printf("Política de substituição (0 - LRU, 1 - Aleatório): ");
    scanf("%d", &substituicao);

    Cache *cache = inicializarCache(tamLinha, numLinhas, associatividade, escrita, substituicao, tempoAcerto);
    Variaveis vars;
    vars.hitsEscrita = vars.hitsLeitura = vars.missesEscrita = vars.missesLeitura = vars.leiturasMP = vars.escritasMP = 0;

    MemoriaPrincipal mem;
    mem.tempoEscrita = 70;
    mem.tempoLeitura = 70;

    /*
    printf("Tempo de leitura da memória principal (ns): ");
    scanf("%d", &mem.tempoLeitura);

    printf("Tempo de escrita da memória principal (ns): ");
    scanf("%d", &mem.tempoEscrita);
    */

    FILE* f = fopen(arquivoEntrada, "r");
    if (f == NULL) {
        printf("Erro ao abrir arquivo de entrada.\n");
        return 0;
    }

    //deixar o endereco direto em int pra manipular o endereco usando operacoes bitwise, em vez de ficar manpulando strings
    int endereco; char operacao;

    //%x scaneia hexa
    while (fscanf(f, "%x %c", &endereco, &operacao) != EOF) {
         acesso(cache, endereco, operacao, &vars);
    }

    fclose(f);

    //o enuncido diz que tem q atualizar a MP após o término da simulação (nos casos de write-back)
    if(cache->politicaEscrita == 1){
        atualizarMP(cache, &vars);
    }
    
    liberarCache(cache);

    return 0;
}

