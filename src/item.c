
#include <stdio.h>
#include <stdlib.h>

#include "../headers/item.h"

enum tipo_coluna { COL_TEXTO, COL_NUM, COL_ANO_DATA };

typedef struct {
  const char *nome;
  int tipo;
  int origem;
} COLUNA;

typedef struct {
  char delim;
  int n_arquivo;
  int n;
  const COLUNA *colunas;
} TEMPLATE;

#define TXT(nome) { nome, COL_TEXTO, -1 }
#define NUM(nome) { nome, COL_NUM, -1 }

static const COLUNA COLUNAS_NASA[] = {
  TXT("asteroid_designation"), TXT("asteroid_fullname"), TXT("close_approach_date"),
  NUM("year"), NUM("month"), NUM("distance_au"), NUM("velocity_km_s"),
  NUM("absolute_magnitude"), NUM("days_until_approach"),
  TXT("is_past_event"), TXT("is_future_event"),
  NUM("risk_score"), NUM("panic_level"),
  TXT("threat_category"), TXT("panic_verdict"), TXT("on_sentry_list"),
  NUM("sentry_impact_prob"), NUM("sentry_torino_scale"),
  NUM("sentry_palermo_scale"), NUM("sentry_diameter_km")
};

static const COLUNA COLUNAS_ANCINE[] = {
  TXT("NOME_SALA"), NUM("REGISTRO_SALA"), TXT("CNPJ_SALA"), TXT("SITUACAO_SALA"),
  TXT("DATA_SITUACAO_SALA"), TXT("DATA_INICIO_FUNCIONAMENTO_SALA"),
  NUM("ASSENTOS_SALA"), NUM("ASSENTOS_CADEIRANTES"),
  NUM("ASSENTOS_MOBILIDADE_REDUZIDA"), NUM("ASSENTOS_OBESIDADE"),
  TXT("ACESSO_ASSENTOS_COM_RAMPA"), TXT("ACESSO_SALA_COM_RAMPA"), TXT("BANHEIROS_ACESSIVEIS"),
  TXT("NOME_COMPLEXO"), NUM("REGISTRO_COMPLEXO"), TXT("SITUACAO_COMPLEXO"),
  TXT("DATA_SITUACAO_COMPLEXO"), TXT("PAGINA_ELETRONICA_COMPLEXO"),
  TXT("ENDERECO_COMPLEXO"), NUM("NUMERO_ENDERECO_COMPLEXO"), TXT("COMPLEMENTO_COMPLEXO"),
  TXT("BAIRRO_COMPLEXO"), TXT("MUNICIPIO_COMPLEXO"), TXT("CEP_COMPLEXO"), TXT("UF_COMPLEXO"),
  TXT("COMPLEXO_ITINERANTE"), TXT("OPERACAO_USUAL"),
  TXT("NOME_EXIBIDOR"), NUM("REGISTRO_EXIBIDOR"), TXT("CNPJ_EXIBIDOR"),
  TXT("SITUACAO_EXIBIDOR"), TXT("NOME_GRUPO_EXIBIDOR"),
  { "ano_inicio_sala", COL_ANO_DATA, 5 }
};

static const TEMPLATE TEMPLATE_NASA = { ',', 20, 20, COLUNAS_NASA };
static const TEMPLATE TEMPLATE_ANCINE = { ';', 32, 33, COLUNAS_ANCINE };

static const char VAZIO[] = "";
#define TAM_ANO 8

struct item_ {
  int id;
  double sim;
  int crlf;
  const TEMPLATE *tpl;
  char *memoria;
  const char **texto;
  double *num;
  unsigned long long tem_num;
  long offset;
};

static int proximo_id = 0;

//funcoes auxiliares
static size_t tamanho(const char *s) {
  size_t n = 0;

  while (s[n] != '\0') n++;

  return n;
}

static void copiar(char *dest, const char *orig) {
  while ((*dest++ = *orig++) != '\0');
}

static int comparar(const char *a, const char *b) {
  while (*a && *a == *b) {
    a++;
    b++;
  }

  return (unsigned char)*a - (unsigned char)*b;
}

static char minuscula(char c) {
  if (c >= 'A' && c <= 'Z')
    return c + ('a' - 'A');

  return c;
}

//compara duas strings ignorando maiusculas e minusculas
static int comparar_sem_maiusculas(const char *a, const char *b) {
  while (*a && *b) {
    char ca = minuscula(*a);
    char cb = minuscula(*b);

    if (ca != cb)
      return (unsigned char)ca - (unsigned char)cb;

    a++;
    b++;
  }

  return (unsigned char)minuscula(*a) - (unsigned char)minuscula(*b);
}

static const char *ultima_ocorrencia(const char *s, char c) {
  const char *resultado = NULL;

  while (*s) {
    if (*s == c) resultado = s;
    s++;
  }

  return resultado;
}

//converte uma string numerica para double, incluindo notacao cientifica
static int para_double(const char *s, double *out) {
  double valor = 0;
  double fator = 0.1;
  int sinal = 1;
  int expoente = 0;
  int sinal_exp = 1;
  int encontrou = 0;

  while (*s == ' ' || *s == '\t') s++;

  if (*s == '-') {
    sinal = -1;
    s++;
  } else if (*s == '+') {
    s++;
  }

  while (*s >= '0' && *s <= '9') {
    valor = valor * 10 + (*s - '0');
    s++;
    encontrou = 1;
  }

  if (*s == '.') {
    s++;

    while (*s >= '0' && *s <= '9') {
      valor += (*s - '0') * fator;
      fator *= 0.1;
      s++;
      encontrou = 1;
    }
  }

  if (!encontrou) return 0;

  if (*s == 'e' || *s == 'E') {
    s++;

    if (*s == '-') {
      sinal_exp = -1;
      s++;
    } else if (*s == '+') {
      s++;
    }

    if (*s < '0' || *s > '9') return 0;

    while (*s >= '0' && *s <= '9') {
      if (expoente < 1000)
        expoente = expoente * 10 + (*s - '0');

      s++;
    }
  }

  while (*s == ' ' || *s == '\t') s++;

  if (*s != '\0') return 0;

  //aplica o expoente da notacao cientifica
  while (expoente > 0) {
    if (sinal_exp > 0) {
      if (valor > 1.7976931348623157e308 / 10.0)
        return 0;

      valor *= 10.0;
    } else {
      valor /= 10.0;
    }

    expoente--;
  }

  valor *= sinal;

  if (valor != valor || valor > 1.7976931348623157e308 ||
      valor < -1.7976931348623157e308)
    return 0;

  *out = valor;
  return 1;
}

//extrai o ano de uma data no formato DD/MM/AAAA
static int ano_de_data(const char *data, double *ano) {
  const char *barra = ultima_ocorrencia(data, '/');

  if (barra == NULL || tamanho(barra + 1) != 4)
    return 0;

  return para_double(barra + 1, ano);
}

//converte um numero inteiro para string
static void inteiro_para_texto(int valor, char *dest) {
  char temp[16];
  int i = 0;
  int j = 0;

  if (valor == 0) {
    dest[0] = '0';
    dest[1] = '\0';
    return;
  }

  if (valor < 0) {
    dest[j++] = '-';
    valor = -valor;
  }

  while (valor > 0) {
    temp[i++] = '0' + valor % 10;
    valor /= 10;
  }

  while (i > 0)
    dest[j++] = temp[--i];

  dest[j] = '\0';
}

//separa os campos da linha, respeitando delimitadores dentro de aspas
static int dividir(char *s, char delim, char **campos, int max) {
  int n = 0, aspas = 0;
  char *w = s;

  campos[n++] = w;

  for (; *s != '\0'; s++) {
    if (*s == '"') {
      if (aspas && s[1] == '"') {
        *w++ = '"';
        s++;
      } else {
        aspas = !aspas;
      }
    } else if (*s == delim && !aspas) {
      *w++ = '\0';

      if (n < max)
        campos[n++] = w;
    } else {
      *w++ = *s;
    }
  }

  *w = '\0';

  return n;
}

//cria um item a partir de uma linha do arquivo
static ITEM* item_de_linha(const TEMPLATE *t, const char *linha, int crlf) {
  size_t len = tamanho(linha);
  size_t n = (size_t)t->n;

  ITEM *item = (ITEM*) malloc(sizeof(ITEM) + n * sizeof(char*) + n * sizeof(double));

  if (item == NULL) return NULL;

  item->memoria = (char*) malloc(2 * (len + 1) +
                    (n - (size_t)t->n_arquivo) * TAM_ANO);

  if (item->memoria == NULL) {
    free(item);
    return NULL;
  }

  item->texto = (const char**)(item + 1);
  item->num = (double*)(item->texto + n);
  item->tpl = t;
  item->crlf = crlf;
  item->sim = 0;
  item->id = ++proximo_id;
  item->tem_num = 0;
  item->offset = -1;

  char *copia = item->memoria;
  char *buffer = copia + len + 1;
  char *extra = buffer + len + 1;
  char *campos[64];

  copiar(copia, linha);
  copiar(buffer, linha);

  int lidos = dividir(buffer, t->delim, campos, t->n_arquivo);

  for (int i = 0; i < t->n; i++) {
    const COLUNA *col = &t->colunas[i];

    item->texto[i] = VAZIO;
    item->num[i] = 0;

    if (col->tipo == COL_ANO_DATA) {
      double ano;

      if (col->origem < lidos &&
          ano_de_data(campos[col->origem], &ano)) {
        inteiro_para_texto((int)ano, extra);
        item->texto[i] = extra;
        item->num[i] = ano;
        item->tem_num |= 1ULL << i;
        extra += TAM_ANO;
      }

      continue;
    }

    if (i < lidos)
      item->texto[i] = campos[i];

    if (col->tipo == COL_NUM &&
        para_double(item->texto[i], &item->num[i]))
      item->tem_num |= 1ULL << i;
  }

  return item;
}

//le uma linha do arquivo e cria o item correspondente
static ITEM* ler_item(FILE *arquivo, const TEMPLATE *t) {
  char *linha = NULL;
  size_t cap = 0;
  size_t len = 0;
  int c;
  int crlf;
  ITEM *item = NULL;

  if (arquivo == NULL) return NULL;

  long pos = ftell(arquivo);

  while (1) {
    c = fgetc(arquivo);

    if (c == EOF || c == '\n')
      break;

    if (len + 1 >= cap) {
      cap = cap == 0 ? 128 : cap * 2;

      char *nova = (char*)realloc(linha, cap);

      if (nova == NULL) {
        free(linha);
        return NULL;
      }

      linha = nova;
    }

    linha[len++] = (char)c;
  }

  if (c == EOF && len == 0) {
    free(linha);
    return NULL;
  }

  crlf = 0;

  if (len > 0 && linha[len - 1] == '\r') {
    len--;
    crlf = 1;
  }

  if (linha == NULL) {
    linha = (char*)malloc(1);

    if (linha == NULL) return NULL;
  }

  linha[len] = '\0';

  //ignora linhas vazias
  if (len == 0) {
    free(linha);

    if (c == EOF) return NULL;

    return ler_item(arquivo, t);
  }

  item = item_de_linha(t, linha, crlf);

  if (item != NULL)
    item->offset = pos;

  free(linha);

  return item;
}

//escreve o registro preservando o tipo de quebra de linha original
static int escrever_item(FILE *arquivo, ITEM *item) {
  if (arquivo == NULL || item == NULL) return 0;

  return fputs(item->memoria, arquivo) != EOF &&
         fputs(item->crlf ? "\r\n" : "\n", arquivo) != EOF;
}

ITEM* item_ler_nasa(FILE *arquivo) {
  return ler_item(arquivo, &TEMPLATE_NASA);
}

int item_escrever_nasa(FILE *arquivo, ITEM *item) {
  return escrever_item(arquivo, item);
}

ITEM* item_ler_ancine(FILE *arquivo) {
  return ler_item(arquivo, &TEMPLATE_ANCINE);
}

int item_escrever_ancine(FILE *arquivo, ITEM *item) {
  return escrever_item(arquivo, item);
}

//cria uma copia do item mantendo seus metadados
ITEM* item_copiar(ITEM *item) {
  if (item == NULL) return NULL;

  ITEM *copia = item_de_linha(item->tpl, item->memoria, item->crlf);

  if (copia == NULL) return NULL;

  copia->id = item->id;
  copia->sim = item->sim;
  copia->offset = item->offset;

  return copia;
}

void item_apagar(ITEM **item) {
  if (item == NULL || *item == NULL) return;

  free((*item)->memoria);
  free(*item);

  *item = NULL;
}

//retorna o indice da coluna no template ou -1 se nao existir
static int indice_campo(ITEM *item, const char *campo) {
  for (int i = 0; i < item->tpl->n; i++)
    if (comparar_sem_maiusculas(item->tpl->colunas[i].nome, campo) == 0)
      return i;

  return -1;
}

int item_get_id(ITEM *item) {
  return item != NULL ? item->id : -1;
}

//retorna o valor numerico da coluna se estiver disponivel
int item_get_num(ITEM *item, const char *campo, double *out) {
  if (item == NULL || campo == NULL || out == NULL)
    return 0;

  int i = indice_campo(item, campo);

  if (i < 0 || !((item->tem_num >> i) & 1ULL))
    return 0;

  *out = item->num[i];

  return 1;
}

//retorna o texto da coluna ou NULL se estiver vazio
const char *item_get_str(ITEM *item, const char *campo) {
  if (item == NULL || campo == NULL)
    return NULL;

  int i = indice_campo(item, campo);

  if (i < 0 || item->texto[i][0] == '\0')
    return NULL;

  return item->texto[i];
}

void item_set_sim(ITEM *item, double sim) {
  if (item != NULL)
    item->sim = sim;
}

double item_get_sim(ITEM *item) {
  return item != NULL ? item->sim : 0;
}

//posicao, em bytes, da linha do registro no arquivo; -1 se o item nao veio de uma leitura
long item_get_offset(ITEM *item) {
  return item != NULL ? item->offset : -1;
}

//compara dois itens pelo valor da coluna 'campo': -1.0 (a < b), 0.0 (iguais) ou 1.0 (a > b)
double item_comparar(ITEM *a, ITEM *b, const char *campo) {
  double x, y;

  if (a == b) return 0.0;
  if (a == NULL) return -1.0;
  if (b == NULL) return 1.0;
  if (campo == NULL) return 0.0;

  if (item_get_num(a, campo, &x) && item_get_num(b, campo, &y))
    return (x > y) - (x < y);

  const char *s = item_get_str(a, campo);
  const char *t = item_get_str(b, campo);

  int c = comparar(s != NULL ? s : VAZIO, t != NULL ? t : VAZIO);

  return (c > 0) - (c < 0);
}

//1 se a coluna e numerica, 0 se e texto, -1 se nao existe no template
static int campo_numerico(const TEMPLATE *t, const char *campo) {
  if (t == NULL || campo == NULL)
    return -1;

  for (int i = 0; i < t->n; i++)
    if (comparar_sem_maiusculas(t->colunas[i].nome, campo) == 0)
      return t->colunas[i].tipo != COL_TEXTO;

  return -1;
}

int item_campo_numerico_nasa(const char *campo) {
  return campo_numerico(&TEMPLATE_NASA, campo);
}

int item_campo_numerico_ancine(const char *campo) {
  return campo_numerico(&TEMPLATE_ANCINE, campo);
}
