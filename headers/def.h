#ifndef DEF_H
#define DEF_H

#include "item.h"

//leitor de um dataset: le o proximo registro como ITEM (chave da coluna `campo` + offset)
typedef ITEM* (*LEITOR_ITEM)(FILE *arquivo, const char *campo);

//procura a coluna no template do dataset: devolve o nome oficial e o tipo da chave, ou NULL
typedef const char *(*CAMPO_ITEM)(const char *campo, TipoChave *tipo);

typedef struct {
    const char *nome;
    LEITOR_ITEM ler;
    CAMPO_ITEM  campo;
} Dataset;

#endif
