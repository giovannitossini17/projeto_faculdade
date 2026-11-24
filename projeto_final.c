/*
 * Projeto Final - Programacao Estruturada
 * Geracao de arquivos RETORNO (um por cliente) a partir de um arquivo REMESSA.
 *
 * Uso:  programa <arquivo_de_remessa.txt>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#define TAM_CABECALHO       9
#define TAM_BOLETO          44
#define TAM_LINHA_MAX       256
#define CAPACIDADE_INICIAL  100
#define MAX_QTD_RETORNO     999999LL      /* cabe em 6 digitos  */
#define MAX_VALOR_RETORNO   9999999999LL  /* cabe em 10 digitos */

/* Um boleto pago, ja "fatiado" em campos */
typedef struct {
    char cliente[6];
    char conta[7];
    char num_boleto[9];
    char data_vencimento[9];
    char data_pagamento[9];
    char valor_pago_str[9];
    long long valor_pago;   /* valor em centavos, para somar sem erro de arredondamento */
} Boleto;

/* ------------------------------------------------------------------ */
/* Tratamento de erro (Regra 1)                                        */
/* ------------------------------------------------------------------ */

/* Funciona como printf: grava a mensagem em ERRO.TXT e encerra o programa. */
void gerar_erro(const char *formato, ...) {
    char mensagem[400];
    va_list args;
    va_start(args, formato);
    vsnprintf(mensagem, sizeof(mensagem), formato, args);
    va_end(args);

    FILE *f_erro = fopen("ERRO.TXT", "w");
    if (f_erro) {
        fprintf(f_erro, "ERRO ENCONTRADO NO ARQUIVO DE REMESSA:\n%s\n", mensagem);
        fclose(f_erro);
    }
    fprintf(stderr, "ERRO: %s\n", mensagem);
    exit(1);   /* o sistema operacional fecha arquivos e libera a memoria */
}

/* ------------------------------------------------------------------ */
/* Funcoes auxiliares de validacao                                     */
/* ------------------------------------------------------------------ */

void limpar_quebra_linha(char *str) {
    int len = (int)strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

int eh_numerico(const char *str) {
    if (str == NULL || str[0] == '\0') return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)str[i])) return 0;
    }
    return 1;
}

/* Valida data no formato AAAAMMDD (inclui ano bissexto) */
int data_valida(const char *data) {
    if (strlen(data) != 8 || !eh_numerico(data)) return 0;

    int ano, mes, dia;
    sscanf(data, "%4d%2d%2d", &ano, &mes, &dia);

    if (ano < 1900 || ano > 2100) return 0;
    if (mes < 1 || mes > 12) return 0;
    if (dia < 1 || dia > 31) return 0;
    if ((mes == 4 || mes == 6 || mes == 9 || mes == 11) && dia > 30) return 0;
    if (mes == 2) {
        int bissexto = (ano % 4 == 0 && (ano % 100 != 0 || ano % 400 == 0));
        if (dia > (bissexto ? 29 : 28)) return 0;
    }
    return 1;
}

void exigir_numerico(const char *nome, const char *valor, int num_linha) {
    if (!eh_numerico(valor)) {
        gerar_erro("Linha %d invalida: campo %s ('%s') deve conter apenas digitos de 0 a 9.",
                   num_linha, nome, valor);
    }
}

void exigir_data(const char *nome, const char *valor, int num_linha) {
    if (!data_valida(valor)) {
        gerar_erro("Linha %d invalida: campo %s ('%s') nao e uma data valida no formato AAAAMMDD.",
                   num_linha, nome, valor);
    }
}

/* ------------------------------------------------------------------ */
/* Leitura de uma linha de boleto                                      */
/* ------------------------------------------------------------------ */

/* Copia 'tamanho' caracteres a partir de 'posicao' e coloca o '\0' final */
void copiar_campo(char *destino, const char *linha, int posicao, int tamanho) {
    memcpy(destino, linha + posicao, tamanho);
    destino[tamanho] = '\0';
}

/* Valida a linha e preenche a struct. Em caso de erro, encerra o programa. */
void ler_boleto(const char *linha, int num_linha, Boleto *b) {
    if (linha[0] != '2') {
        gerar_erro("Linha %d invalida: o ID do registro deve ser 2, mas foi encontrado '%c'.",
                   num_linha, linha[0]);
    }
    int tamanho = (int)strlen(linha);
    if (tamanho != TAM_BOLETO) {
        gerar_erro("Linha %d com tamanho incorreto: esperado %d caracteres, encontrado %d.",
                   num_linha, TAM_BOLETO, tamanho);
    }

    copiar_campo(b->cliente,         linha,  1, 5);
    copiar_campo(b->conta,           linha,  6, 6);
    copiar_campo(b->num_boleto,      linha, 12, 8);
    copiar_campo(b->data_vencimento, linha, 20, 8);
    copiar_campo(b->data_pagamento,  linha, 28, 8);
    copiar_campo(b->valor_pago_str,  linha, 36, 8);

    exigir_numerico("CLIENTE",     b->cliente,     num_linha);
    exigir_numerico("CONTA",       b->conta,       num_linha);
    exigir_numerico("NUM_BOLETO",  b->num_boleto,  num_linha);
    exigir_data("DATA_VENC",       b->data_vencimento, num_linha);
    exigir_data("DATA_PAGAMENTO",  b->data_pagamento,  num_linha);
    exigir_numerico("VALOR_PAGO",  b->valor_pago_str,  num_linha);

    b->valor_pago = atoll(b->valor_pago_str);
}

/* ------------------------------------------------------------------ */
/* Geracao dos arquivos de retorno (Regra 2)                           */
/* ------------------------------------------------------------------ */

/* Conta os boletos e soma os valores de um cliente */
void totalizar_cliente(const Boleto *boletos, int total, const char *cliente,
                       int *qtd, long long *soma) {
    *qtd = 0;
    *soma = 0;
    for (int i = 0; i < total; i++) {
        if (strcmp(boletos[i].cliente, cliente) == 0) {
            (*qtd)++;
            *soma += boletos[i].valor_pago;
        }
    }
}

/* Retorna 1 se o cliente do boleto 'indice' ainda nao apareceu antes na lista */
int primeira_ocorrencia(const Boleto *boletos, int indice) {
    for (int j = 0; j < indice; j++) {
        if (strcmp(boletos[j].cliente, boletos[indice].cliente) == 0) return 0;
    }
    return 1;
}

void gerar_retorno(const Boleto *boletos, int total, const char *cliente,
                   const char *data_geracao) {
    char nome_arquivo[32];
    snprintf(nome_arquivo, sizeof(nome_arquivo), "%s_%s.txt", data_geracao, cliente);

    FILE *f_out = fopen(nome_arquivo, "w");
    if (!f_out) {
        gerar_erro("Nao foi possivel criar o arquivo de retorno '%s' no diretorio atual.",
                   nome_arquivo);
    }

    int qtd;
    long long soma;
    totalizar_cliente(boletos, total, cliente, &qtd, &soma);

    /* Cabecalho: 1 + data (9 caracteres) */
    fprintf(f_out, "1%s\n", data_geracao);

    /* Boletos do cliente: 2 + conta + boleto + data pgto + valor (31 caracteres) */
    for (int j = 0; j < total; j++) {
        if (strcmp(boletos[j].cliente, cliente) == 0) {
            fprintf(f_out, "2%s%s%s%s\n",
                    boletos[j].conta,
                    boletos[j].num_boleto,
                    boletos[j].data_pagamento,
                    boletos[j].valor_pago_str);
        }
    }

    /* Rodape: 3 + total (6 digitos) + soma (10 digitos) = 17 caracteres */
    fprintf(f_out, "3%06d%010lld\n", qtd, soma);
    fclose(f_out);
}

/* ------------------------------------------------------------------ */
/* Programa principal                                                  */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    /* Regra 3: tudo via linha de comando */
    if (argc != 2) {
        gerar_erro("Uso incorreto. Execute: programa <arquivo_de_remessa.txt>");
    }

    FILE *f_in = fopen(argv[1], "r");
    if (!f_in) {
        gerar_erro("Nao foi possivel abrir o arquivo de remessa '%s'.", argv[1]);
    }

    char linha[TAM_LINHA_MAX];

    /* ---------- Linha 1: cabecalho ---------- */
    if (!fgets(linha, sizeof(linha), f_in)) {
        gerar_erro("O arquivo de remessa esta vazio.");
    }
    limpar_quebra_linha(linha);

    /* Ignora o BOM do UTF-8 (3 bytes invisiveis que o Bloco de Notas pode gravar) */
    char *cab = linha;
    if ((unsigned char)cab[0] == 0xEF && (unsigned char)cab[1] == 0xBB &&
        (unsigned char)cab[2] == 0xBF) {
        cab += 3;
    }

    if (cab[0] != '1') {
        gerar_erro("Linha 1 (cabecalho) invalida: o ID deve ser 1, mas foi encontrado '%c'.", cab[0]);
    }
    if (strlen(cab) != TAM_CABECALHO) {
        gerar_erro("Linha 1 (cabecalho) com tamanho incorreto: esperado %d caracteres, encontrado %d.",
                   TAM_CABECALHO, (int)strlen(cab));
    }
    if (!data_valida(cab + 1)) {
        gerar_erro("Linha 1 (cabecalho) invalida: a data da remessa ('%s') deve ser uma data valida AAAAMMDD.",
                   cab + 1);
    }

    /* ---------- Demais linhas: boletos pagos ---------- */
    int capacidade = CAPACIDADE_INICIAL;
    int total_boletos = 0;
    Boleto *boletos = malloc(capacidade * sizeof(Boleto));
    if (!boletos) {
        gerar_erro("Falha interna de alocacao de memoria.");
    }

    int num_linha = 2;
    int primeira_branca = 0;   /* numero da primeira linha em branco ainda "pendente" */

    while (fgets(linha, sizeof(linha), f_in)) {
        limpar_quebra_linha(linha);

        /* Linhas em branco so sao toleradas no FIM do arquivo */
        if (linha[0] == '\0') {
            if (primeira_branca == 0) primeira_branca = num_linha;
            num_linha++;
            continue;
        }
        if (primeira_branca != 0) {
            gerar_erro("Linha %d invalida: linha em branco no meio do arquivo de remessa.",
                       primeira_branca);
        }

        /* Cresce o vetor quando enche (dobra a capacidade) */
        if (total_boletos == capacidade) {
            capacidade *= 2;
            Boleto *novo = realloc(boletos, capacidade * sizeof(Boleto));
            if (!novo) {
                gerar_erro("Falha interna de alocacao de memoria.");
            }
            boletos = novo;
        }

        ler_boleto(linha, num_linha, &boletos[total_boletos]);
        total_boletos++;
        num_linha++;
    }
    fclose(f_in);

    if (total_boletos == 0) {
        gerar_erro("Arquivo de remessa invalido: nenhum boleto encontrado apos o cabecalho.");
    }

    /* ---------- Confere se os totais cabem nos campos do rodape ---------- */
    for (int i = 0; i < total_boletos; i++) {
        if (!primeira_ocorrencia(boletos, i)) continue;

        int qtd;
        long long soma;
        totalizar_cliente(boletos, total_boletos, boletos[i].cliente, &qtd, &soma);

        if (qtd > MAX_QTD_RETORNO) {
            gerar_erro("Cliente %s possui %d boletos: excede o limite de 6 digitos do campo TOTAL_BOLETOS.",
                       boletos[i].cliente, qtd);
        }
        if (soma > MAX_VALOR_RETORNO) {
            gerar_erro("Cliente %s: a soma dos valores excede o limite de 10 digitos do campo VALOR_TOTAL.",
                       boletos[i].cliente);
        }
    }

    /* Tudo certo: apaga um ERRO.TXT antigo, se existir */
    remove("ERRO.TXT");

    /* ---------- Data de geracao (hoje) em AAAAMMDD ---------- */
    time_t agora = time(NULL);
    char data_geracao[9];
    strftime(data_geracao, sizeof(data_geracao), "%Y%m%d", localtime(&agora));

    /* ---------- Um arquivo de retorno por cliente ---------- */
    int arquivos = 0;
    for (int i = 0; i < total_boletos; i++) {
        if (primeira_ocorrencia(boletos, i)) {
            gerar_retorno(boletos, total_boletos, boletos[i].cliente, data_geracao);
            arquivos++;
        }
    }

    printf("Processamento concluido: %d boleto(s) lido(s), %d arquivo(s) de retorno gerado(s).\n",
           total_boletos, arquivos);

    free(boletos);
    return 0;
}