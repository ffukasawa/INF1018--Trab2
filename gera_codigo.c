/* 
   Trab 2 - INF1018
   Fernanda Fukasawa Amarante 2410444 3WA
   Filipe Izidoro Reis 2410329 3WC
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gera_codigo.h"

#define MAX 50

/*
v0 -> -4(%rbp)
v1 -> -8(%rbp)
v2 -> -12(%rbp)
v3 -> -16(%rbp)
v4 -> -20(%rbp)
p0 -> -24(%rbp)
*/

//------------------------------------------------funcao de erro------------------------------------------------
static void error (const char *msg, int line) {
    fprintf(stderr, "erro %s na linha %d\n", msg, line);
    exit(EXIT_FAILURE);
}

//------------------------------------------------funcao de byte------------------------------------------------
static void emite_byte(unsigned char cod[], int *ppos, unsigned char b){
  cod[*ppos] = b;    // coloca o byte na posicao atual 
  (*ppos)++;         // avanca para a proxima posicao 
}

//------------------------------------------------funcao de 4 bytes------------------------------------------------
static void emite_int(unsigned char cod[], int *ppos, int v){
  int *p = (int *)&cod[*ppos];
  *p = v;            // grava o int v em 4 bytes a partir de cod[pos] 
  *ppos += 4;        // anda 4 posicoes 
}

//------------------------------------------------funcao de prologo------------------------------------------------
static void gera_prologo(unsigned char cod[], int *ppos){
  //push %rbp
  emite_byte(cod, ppos, 0x55);
  //movq %rsp, %rbp
  emite_byte(cod, ppos, 0x48);
  emite_byte(cod, ppos, 0x89);
  emite_byte(cod, ppos, 0xe5);
  //subq $32, %rsp    espaco para 5 variaveis locais e 1 de saida (p0)
  emite_byte(cod, ppos, 0x48);
  emite_byte(cod, ppos, 0x83);
  emite_byte(cod, ppos, 0xec);
  emite_byte(cod, ppos, 0x20);
  // mov %edi, -24(%rbp)    guarda p0, protegendo de calls
  emite_byte(cod, ppos, 0x89);
  emite_byte(cod, ppos, 0x7d);
  emite_byte(cod, ppos, 0xe8);
}

//------------------------------------------------funcao de epilogo------------------------------------------------
static void gera_epilogo(unsigned char cod[], int *ppos) {
  emite_byte(cod, ppos, 0xc9);  // leave
  emite_byte(cod, ppos, 0xc3);  // ret
}

//------------------------------------------------funcao de carregar------------------------------------------------
static void carrega_varpc_em_eax(unsigned char cod[], int *ppos, char var, int idx){
   if (var == '$') {
        // mov $idx, %eax
        emite_byte(cod, ppos, 0xb8);   
        emite_int (cod, ppos, idx);     // constante em 4 bytes
    }
    else if (var == 'v') {
        // mov offset(%rbp), %eax
        int offset = -4 * (idx + 1);    // v0:-4, v1:-8, v2:-12, ...
        emite_byte(cod, ppos, 0x8b);
        emite_byte(cod, ppos, 0x45);
        emite_byte(cod, ppos, (unsigned char)offset);
    }
    else if (var == 'p') {
        // somente p0 existe -> offset = -24
        emite_byte(cod, ppos, 0x8b);
        emite_byte(cod, ppos, 0x45);
        emite_byte(cod, ppos, 0xe8);    // -24 = 0xe8
    }
else {
        error("operando invalido no carregamento em EAX", 0);
    }

}

//------------------------------------------------funcao de salvar------------------------------------------------
static void salva_eax_em_var(unsigned char cod[], int *ppos, char var, int idx){
  if (var == 'v') {
    int offset = -4 * (idx + 1);
  //mov eax, idx
    emite_byte(cod, ppos, 0x89); 
    emite_byte(cod, ppos, 0x45); 
    emite_byte(cod, ppos, (unsigned char)offset);
  } else if (var == 'p') {
// guarda o valor de eax em p0
    
    emite_byte(cod, ppos, 0x89); 
    emite_byte(cod, ppos, 0x45); 
    emite_byte(cod, ppos, 0xe8);
  } else {
    error("destino invalido no salvamento de EAX", 0);
  }
}


//------------------------------funcao de operacoes------------------------------------------------
static void gera_oper(unsigned char cod[], int *ppos,
                      char var0, int idx0,
                      char var1, int idx1,
                      char op, char var2, int idx2)
{
/* var0 = var1 op var2 */

  /* carrega var1 em eax */
  carrega_varpc_em_eax(cod, ppos, var1, idx1);

  /* carrega var2 em ecx */
  if (var2 == 'v') {
      int offset = -4 * (idx2 + 1);
      emite_byte(cod, ppos, 0x8B); 
      emite_byte(cod, ppos, 0x4D); 
      emite_byte(cod, ppos, (unsigned char)offset); /* carrega variável local em ecx */
  } 
  else if (var2 == 'p') {
      emite_byte(cod, ppos, 0x8B); 
      emite_byte(cod, ppos, 0x4D); 
      emite_byte(cod, ppos, 0xE8); /* carrega p0 em ecx */
  } 
  else if (var2 == '$') {
      emite_byte(cod, ppos, 0xB9); 
      emite_int(cod, ppos, idx2); /* ecx = constante */
  } 
  else {
      error("operando invalido no load em ecx", 0);
  }

  /* aplica operacao: */
  if (op == '+'){
    /* add eax, ecx  -> 01 C8 */
    emite_byte(cod, ppos, 0x01); 
    emite_byte(cod, ppos, 0xC8);
  }
  else if (op == '-'){
    /* sub eax, ecx -> 29 C8 */
    emite_byte(cod, ppos, 0x29); 
    emite_byte(cod, ppos, 0xC8);
  }
  else if (op == '*'){
    /* imul eax, ecx -> 0F AF C1 */
    emite_byte(cod, ppos, 0x0F); 
    emite_byte(cod, ppos, 0xAF); 
    emite_byte(cod, ppos, 0xC1);
  }
  else {
    error("operador desconhecido", 0);
  }

  /* salva eax em var0 */
  salva_eax_em_var(cod, ppos, var0, idx0);
}

//------------------------------------------------funcao de retorno------------------------------------------------
static void gera_ret(unsigned char cod[], int *ppos, char var0, int idx0) {
    carrega_varpc_em_eax(cod, ppos, var0, idx0);
    gera_epilogo(cod, ppos);
}

//----------------------------------------------funcao de z_retorno------------------------------------------------
static void gera_zret(unsigned char cod[], int *ppos,
                      char var0, int idx0,
                      char var1, int idx1)
{
  /* if var0 == 0 entao retorna var1 */

  /* calcular tamanho das instrucoes que serao puladas (load var1 + epilogo) */
  int set_size;
  if (var1 == 'v') set_size = 3;       /* carrega a variável local em EAX -> 8b 45 xx*/
  else if (var1 == 'p') set_size = 3;  /* carrega p0 em EAX -> 8b 45 xx*/
  else set_size = 5;                    /* coloca a constante em EAX -> b8 xx xx xx xx */
  int epilogue_size = 2; /* leave (c9); ret (c3)*/
  int skip = set_size + epilogue_size;

  /* carrega var0 e compara com zero */
  if (var0 == 'v') {
    carrega_varpc_em_eax(cod, ppos, var0, idx0);
    /* cmp eax, 0 -> 83 F8 00 */
    emite_byte(cod, ppos, 0x83); 
    emite_byte(cod, ppos, 0xF8); 
    emite_byte(cod, ppos, 0x00);
  } 
  else if (var0 == 'p') {
    /* carrega p0 em EAX e compara com zero */
    emite_byte(cod, ppos, 0x8B); 
    emite_byte(cod, ppos, 0x45); 
    emite_byte(cod, ppos, 0xE8); /* coloca p0 em eax */

    emite_byte(cod, ppos, 0x83); 
    emite_byte(cod, ppos, 0xF8); 
    emite_byte(cod, ppos, 0x00); /* cmp eax,0 */
  } 
  else { /* const */
    carrega_varpc_em_eax(cod, ppos, var0, idx0);
    emite_byte(cod, ppos, 0x83); 
    emite_byte(cod, ppos, 0xF8); 
    emite_byte(cod, ppos, 0x00); /* cmp eax,0 */
  }

  /* jne skip */
  emite_byte(cod, ppos, 0x75); 
  emite_byte(cod, ppos, (unsigned char)skip);

  /* a == 0: carregar b (var1) em eax e retornar */
  carrega_varpc_em_eax(cod, ppos, var1, idx1);
  gera_epilogo(cod, ppos);
}


//------------------------------------------------funcao de call------------------------------------------------
static void gera_call(unsigned char cod[], int *ppos,
                             char var0, int idx0,
                             int num_func,
                             char var1, int idx1,
                             long end_funcoes[])
{
  if (var1 == 'v') {
    int offset = -4 * (idx1 + 1);
    emite_byte(cod, ppos, 0x8B); 
    emite_byte(cod, ppos, 0x7D); 
    emite_byte(cod, ppos, (unsigned char)offset); /* coloca a variável local em EDI */

  } 
  else if (var1 == 'p') {
    /* coloca p0 em EDI -> mov -24(%rbp), %edi */
    emite_byte(cod, ppos, 0x8B); 
    emite_byte(cod, ppos, 0x7D); 
    emite_byte(cod, ppos, 0xE8);
  } 
  else if (var1 == '$') {
    emite_byte(cod, ppos, 0xBF); 
    emite_int(cod, ppos, idx1); /* mov edi, idx1 */
  } 
  else {
    error("operando invalido no call", 0);
  }

 /* ajusta a pilha para manter o alinhamento -> sub rsp, 8*/
  emite_byte(cod, ppos, 0x48); 
  emite_byte(cod, ppos, 0x83); 
  emite_byte(cod, ppos, 0xEC); 
  emite_byte(cod, ppos, 0x08);

  /* chama a função */
  emite_byte(cod, ppos, 0xE8);
  /* calcula rel usando endereco absoluto armazenado em end_funcoes */
  long endereco_funcao = end_funcoes[num_func]; /* endereco onde a func começa */
  long rip = (long)&cod[*ppos] + 4; /* proxima instrucao */

  int rel = (int)(endereco_funcao - rip);
  emite_int(cod, ppos, rel);

  /* restaura o valor original da pilha -> addq $8, %rsp*/
  emite_byte(cod, ppos, 0x48); 
  emite_byte(cod, ppos, 0x83); 
  emite_byte(cod, ppos, 0xC4); 
  emite_byte(cod, ppos, 0x08);

  /* resultado em eax -> armazenar em var0 */
  salva_eax_em_var(cod, ppos, var0, idx0);
}


//--------------------------------------------funcao de compilação------------------------------------------------
void gera_codigo (FILE *f, unsigned char code[], funcp *entry) {
  int line = 1;
  int c;
  int pos = 0;                 // posicao atual de escrita em 'code' 
  long end_funcoes[MAX];       // endereco de inicio de cada funcao gerada 
  int func_atual = 0;          // indice da funcao que estamos lendo 
  int inicio_ultima_funcao = 0;// offset da ultima funcao lida em 'code' 
    
  while ((c = fgetc(f)) != EOF) {

    switch (c) {

        case 'f': { /* function */
            char c0;
            if (fscanf(f, "unction%c", &c0) != 1)
                error("comando invalido", line);

            inicio_ultima_funcao = pos;
            end_funcoes[func_atual++] = (long)&code[pos];

            gera_prologo(code, &pos); // gera o prologo
            break;
        }

        case 'e': { /* end */
            char c0;
            if (fscanf(f, "nd%c", &c0) != 1)
                error("comando invalido", line);
            break;
        }

        case 'r': {  /* retorno incondicional */
            int idx0;
            char var0;

            if (fscanf(f, "et %c%d", &var0, &idx0) != 2)
                error("comando invalido", line);

            gera_ret(code, &pos, var0, idx0); // gera o retorno incondicional
            break;
        }

        case 'z': {  /* retorno condicional */
            int idx0, idx1;
            char var0, var1;

            if (fscanf(f, "ret %c%d %c%d", &var0, &idx0, &var1, &idx1) != 4)
                error("comando invalido", line);

            gera_zret(code, &pos, var0, idx0, var1, idx1); // gera o retorno condicional
            break;
        }

        case 'v': {  /* atribuicao */
            int idx0;
            char var0 = c, c0;

            if (fscanf(f, "%d = %c", &idx0, &c0) != 2)
                error("comando invalido", line);

            if (c0 == 'c') { /* call */

                int fun, idx1;
                char var1;

                if (fscanf(f, "all %d %c%d\n", &fun, &var1, &idx1) != 3)
                    error("comando invalido", line);

                gera_call(code, &pos, var0, idx0, fun, var1, idx1, end_funcoes); // gera o call
            }

            else { /* operacao aritmetica */

                int idx1, idx2;
                char var1 = c0, var2, op;

                if (fscanf(f, "%d %c %c%d", &idx1, &op, &var2, &idx2) != 4)
                    error("comando invalido", line);

                gera_oper(code, &pos, var0, idx0, var1, idx1, op, var2, idx2); // gera a operacao
            }

            break;
        }

        default:
            error("comando desconhecido", line);
    }

    line++;
    fscanf(f, " ");
  }

  *entry = (funcp)(code + inicio_ultima_funcao);
  }


