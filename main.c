/* main.c - Programa de teste para gera_codigo
 *
 * Uso:
 *   gcc -Wall -Wa,--execstack -o testgerador main.c gera_codigo.c
 *   ./testgerador arquivo.lbs [arg]
 *
 * Se [arg] não for fornecido, usa 5 como padrão.
 */

#include <stdio.h>
#include <stdlib.h>
#include "gera_codigo.h"  /* NÃO MODIFICAR este header */

int main(int argc, char *argv[]) {
    FILE *fp;
    funcp funcLBS = NULL;
    unsigned char code[65536]; /* buffer onde gera_codigo irá escrever o código gerado */
    int arg = 5;               /* argumento padrão para a função gerada */
    int res;

    if (argc < 2) {
        fprintf(stderr, "Uso: %s arquivo.lbs [arg]\n", argv[0]);
        return 1;
    }

    if (argc >= 3) {
        arg = atoi(argv[2]);
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror("fopen");
        return 1;
    }

    /* Limpa buffer (opcional, facilita depuração) */
    for (size_t i = 0; i < sizeof(code); ++i) code[i] = 0xCC;

    /* Gera o código. Espera-se que gera_codigo escreva no vetor code
       e coloque em funcLBS o endereço da função a ser chamada. */
    gera_codigo(fp, code, &funcLBS);

    fclose(fp);

    if (funcLBS == NULL) {
        fprintf(stderr, "gera_codigo retornou NULL em entry -> erro na geracao\n");
        return 1;
    }

    /* Chama a função gerada usando o argumento informado */
    res = (*funcLBS)(arg);

    printf("Resultado da funcao gerada(%d) = %d\n", arg, res);

    return 0;
}
