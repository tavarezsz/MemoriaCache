#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//todos os tempos são em ns
#define HITTIME 5
#define RWTIME 70 //read/write time na memoria principal

typedef struct{
    int tag;
    int valido; //indica se o bloco foi carregado da memoria, para evitar ler lixo no inicio do programa quando a cache 
                //ainda não foi totalmente preenchida
    int modificado;
    int ultimoUso; //para o LRU
} Bloco;

typedef struct{
    Bloco* blocos;
} Conjunto;

typedef struct {
    Conjunto* conjuntos;
    int numConjuntos;
    int tamConjunto;
    int tamBloco;
} Cache;

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

int main(){

    int escrita,tamLinha,numLinha,associacao,subtituicao;
 
    printf("Politica de escrita(0 - write-through e 1 - write-back): ");
    scanf("%d",&escrita);
    printf("Tamanho da linha: ");
    scanf("%d",&tamLinha);
    printf("Numero de linhas da cache: ");
    scanf("%d",&numLinha);
    printf("Numero de linhas por conjunto: ");
    scanf("%d",&associacao);
    printf("Politica de subtituicao(0 - LRU e 1 - Aleatoria)");
    scanf("%d",&subtituicao);

    //inicializar a cache
    int qtdConjuntos = (numLinha/tamLinha)/associacao; //total de blocos/blocos por conjunto
    Cache cache; 
    cache.numConjuntos = qtdConjuntos;
    cache.tamConjunto = associacao;
    cache.tamBloco = tamLinha;

    cache.conjuntos = (Conjunto*)malloc(sizeof(Conjunto) * cache.numConjuntos);

    for(int i=0;i<cache.numConjuntos;i++){//alocando espaço para cada bloco de cada conjunto
        cache.conjuntos[i].blocos = (Bloco *)malloc(sizeof(Bloco) * cache.tamConjunto);
        for (int j = 0; j < cache.tamConjunto; j++) 
        {
            cache.conjuntos[i].blocos[j].valido = 0;
            cache.conjuntos[i].blocos[j].tag=-1;
            cache.conjuntos[i].blocos[j].modificado = 0;
            cache.conjuntos[i].blocos[j].ultimoUso=0;
        }
    }

    acesso(&cache,"07b243a0 R"); //so para testar 

    return 0;
}

