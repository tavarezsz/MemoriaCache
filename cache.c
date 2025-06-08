#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
    unsigned int tag;
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
    int hitTime;
} Cache;

//tem que ser configurável pelo usuário, mesmo q pros testes seja sempre 70ns
typedef struct {
    int tempoEscrita;
    int tempoLeitura;
} MemoriaPrincipal;

void acesso(Cache *cache, char entrada[11]){
    char endereco[9];
    char operacao;
    int j=0;

    //separar a string de endereco e operação
        for(int i=0;i<11;i++){
            printf("\nChar atual %c\n",entrada[i]);
            
            if(entrada[i] != ' '){
                endereco[j++] = entrada[i];
            }
            else{
                operacao = entrada[i+1];
                break;
            }
        }
        endereco[j] = '\0';

        printf("Endereco: %s, operacao: %c",endereco,operacao);

    //buscar se esta na cache

    //se não estiver fazer a substituição

}

Cache* inicializarCache(int tamBloco, int numLinhas, int associatividade, 
                 int politicaEscrita, int politicaSubstituicao, int tempoAcerto) {
    Cache *cache = (Cache*)malloc(sizeof(Cache));
    
    cache->numConjuntos = numLinhas / associatividade;
    cache->tamConjunto = associatividade;
    cache->tamBloco = tamBloco;
    cache->politicaEscrita = politicaEscrita;
    cache->politicaSubstituicao = politicaSubstituicao;
    cache->hitTime = tempoAcerto;
    
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


int main(){

    int escrita, tamLinha, numLinhas, associatividade, tempoAcerto, substituicao;
 
    char arquivoEntrada[100];
    char arquivoSaida[100];

    //por enquanto vai ser o de teste

    //printf("Digite o nome do arquivo de entrada: ");
    //scanf("%s", arquivoEntrada);

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

    MemoriaPrincipal mem;
    mem.tempoEscrita = 70;
    mem.tempoLeitura = 70;

    /*
    printf("Tempo de leitura da memória principal (ns): ");
    scanf("%d", &mem.tempoLeitura);

    printf("Tempo de escrita da memória principal (ns): ");
    scanf("%d", &mem.tempoEscrita);
    */

    //acesso(&cache,"07b243a0 R"); 

    return 0;
}

