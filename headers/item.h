#ifndef ITEM_H
#define ITEM_H

#include <stdio.h>

#define ITEM_TXT_MAX 64

typedef struct item_ ITEM;

typedef enum {
  CHAVE_NUM = 0,
  CHAVE_TXT = 1
} TipoChave;

typedef int (*VISITA_ITEM)(ITEM *item, void *ctx);

ITEM* item_ler_nasa(FILE *arquivo);
int item_escrever_nasa(FILE *arquivo, ITEM *item);

ITEM* item_ler_ancine(FILE *arquivo);
int item_escrever_ancine(FILE *arquivo, ITEM *item);

ITEM* item_copiar(ITEM *item);
void item_apagar(ITEM **item);

int item_get_id(ITEM *item);
int item_get_num(ITEM *item, const char *campo, double *out);
const char *item_get_str(ITEM *item, const char *campo);

// sim = similaridade
void item_set_sim(ITEM *item, double sim);
double item_get_sim(ITEM *item);

long item_get_offset(ITEM *item);

double item_comparar(ITEM *a, ITEM *b, const char *campo);

int item_campo_numerico_nasa(const char *campo);
int item_campo_numerico_ancine(const char *campo);

#endif
