#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_BOLETOS 5000 // Limite de registros em memória

// Estrutura para extrair e armazenar cada campo do boleto
typedef struct {
    char cliente[6];
    char conta[7];
    char num_boleto[9];
    char data_vencimento[9];
    char data_pagamento[9];
    char valor_pago_str[9];
    long long valor_pago; // Facilita a soma sem erros de flutuação
    int processado;       // Flag para o momento da geração agrupada
} Boleto;

// Remove quebras de linha (\r e \n) do final das strings de leitura
void limpar_quebra_linha(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

// Verifica se a string contém apenas dígitos numéricos (0-9)
int eh_numerico(const char *str) {
    if (str == NULL || str[0] == '\0') return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)str[i])) {
            return 0;
        }
    }
    return 1;
}

// Verifica se uma data no formato AAAAMMDD é válida
int data_valida(const char *data) {
    if (!eh_numerico(data) || strlen(data) != 8) return 0;

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

// Cria o arquivo ERRO.TXT e aborta o programa (Regra 1)
void gerar_erro(const char *mensagem) {
    FILE *f_erro = fopen("ERRO.TXT", "w");
    if (f_erro) {
        fprintf(f_erro, "ERRO ENCONTRADO NO ARQUIVO DE REMESSA:\n%s\n", mensagem);
        fclose(f_erro);
    }
    exit(1);
}

int main(int argc, char *argv[]) {
    // Regra 3: Operação estritamente via linha de comando, sem menus interativos
    if (argc != 2) {
        gerar_erro("Uso incorreto via linha de comando. Execute: programa <arquivo_de_remessa.txt>");
    }

    FILE *f_in = fopen(argv[1], "r");
    if (!f_in) {
        gerar_erro("Nao foi possivel abrir o arquivo de remessa especificado.");
    }

    char linha[256];

    // Checagem da Linha 1 (Cabeçalho da Remessa)
    if (!fgets(linha, sizeof(linha), f_in)) {
        fclose(f_in);
        gerar_erro("Arquivo de remessa encontra-se vazio.");
    }
    limpar_quebra_linha(linha);

    // Validação do tamanho, ID e data do cabeçalho (9 caracteres, início '1' + data AAAAMMDD)
    if (strlen(linha) != 9 || linha[0] != '1') {
        fclose(f_in);
        gerar_erro("Linha 1 (Cabecalho) fora do padrao: esperado 9 caracteres iniciando com ID=1.");
    }
    if (!data_valida(linha + 1)) {
        fclose(f_in);
        gerar_erro("Linha 1 (Cabecalho) invalida: a data da remessa deve estar no formato valido AAAAMMDD.");
    }

    Boleto *boletos = malloc(MAX_BOLETOS * sizeof(Boleto));
    if (!boletos) {
        fclose(f_in);
        gerar_erro("Falha interna de alocacao de memoria.");
    }

    int total_boletos = 0;
    int num_linha = 2; // Cabeçalho foi lido na linha 1
    char msg_erro[200];

    // Checagem e Parse das Linhas Seguintes (Boletos Pagos)
    while (fgets(linha, sizeof(linha), f_in)) {
        limpar_quebra_linha(linha);

        // Linha em branco no meio/corpo do arquivo caracteriza quebra de layout
        if (strlen(linha) == 0) {
            free(boletos);
            fclose(f_in);
            sprintf(msg_erro, "Linha %d invalida: linha em branco nao permitida no arquivo de remessa.", num_linha);
            gerar_erro(msg_erro);
        }

        // Validação do ID e tamanho da linha de boleto (44 caracteres, início '2')
        if (linha[0] != '2') {
            free(boletos);
            fclose(f_in);
            sprintf(msg_erro, "Linha %d invalida: o ID do registro deve ser 2.", num_linha);
            gerar_erro(msg_erro);
        }
        if (strlen(linha) != 44) {
            free(boletos);
            fclose(f_in);
            sprintf(msg_erro, "Linha %d com tamanho incorreto: esperado 44 caracteres, encontrado %zu.", num_linha, strlen(linha));
            gerar_erro(msg_erro);
        }

        // Fatiamento dos campos utilizando as posições fixas do projeto
        strncpy(boletos[total_boletos].cliente, linha + 1, 5);
        boletos[total_boletos].cliente[5] = '\0';

        strncpy(boletos[total_boletos].conta, linha + 6, 6);
        boletos[total_boletos].conta[6] = '\0';

        strncpy(boletos[total_boletos].num_boleto, linha + 12, 8);
        boletos[total_boletos].num_boleto[8] = '\0';

        strncpy(boletos[total_boletos].data_vencimento, linha + 20, 8);
        boletos[total_boletos].data_vencimento[8] = '\0';

        strncpy(boletos[total_boletos].data_pagamento, linha + 28, 8);
        boletos[total_boletos].data_pagamento[8] = '\0';

        strncpy(boletos[total_boletos].valor_pago_str, linha + 36, 8);
        boletos[total_boletos].valor_pago_str[8] = '\0';

        // Validação estrita dos tipos numéricos e datas dos campos extraídos
        if (!eh_numerico(boletos[total_boletos].cliente)) {
            free(boletos);
            fclose(f_in);
            sprintf(msg_erro, "Linha %d invalida: campo CLIENTE ('%s') deve conter apenas numeros.", num_linha, boletos[total_boletos].cliente);
            gerar_erro(msg_erro);
        }
        if (!eh_numerico(boletos[total_boletos].conta)) {
            free(boletos);
            fclose(f_in);
            sprintf(msg_erro, "Linha %d invalida: campo CONTA ('%s') deve conter apenas numeros.", num_linha, boletos[total_boletos].conta);
            gerar_erro(msg_erro);
        }
        if (!eh_numerico(boletos[total_boletos].num_boleto)) {
            free(boletos);
            fclose(f_in);
            sprintf(msg_erro, "Linha %d invalida: campo NUM_BOLETO ('%s') deve conter apenas numeros.", num_linha, boletos[total_boletos].num_boleto);
            gerar_erro(msg_erro);
        }
        if (!data_valida(boletos[total_boletos].data_vencimento)) {
            free(boletos);
            fclose(f_in);
            sprintf(msg_erro, "Linha %d invalida: campo DATA_VENC ('%s') nao e uma data valida (AAAAMMDD).", num_linha, boletos[total_boletos].data_vencimento);
            gerar_erro(msg_erro);
        }
        if (!data_valida(boletos[total_boletos].data_pagamento)) {
            free(boletos);
            fclose(f_in);
            sprintf(msg_erro, "Linha %d invalida: campo DATA_PAGAMENTO ('%s') nao e uma data valida (AAAAMMDD).", num_linha, boletos[total_boletos].data_pagamento);
            gerar_erro(msg_erro);
        }
        if (!eh_numerico(boletos[total_boletos].valor_pago_str)) {
            free(boletos);
            fclose(f_in);
            sprintf(msg_erro, "Linha %d invalida: campo VALOR_PAGO ('%s') deve conter apenas numeros.", num_linha, boletos[total_boletos].valor_pago_str);
            gerar_erro(msg_erro);
        }

        boletos[total_boletos].valor_pago = atoll(boletos[total_boletos].valor_pago_str);
        boletos[total_boletos].processado = 0;

        total_boletos++;
        num_linha++;

        if (total_boletos >= MAX_BOLETOS) {
            free(boletos);
            fclose(f_in);
            gerar_erro("Limite maximo de capacidade de boletos (5000) excedido.");
        }
    }
    fclose(f_in);

    // Garante que o arquivo possui pelo menos um boleto registrado além do cabeçalho
    if (total_boletos == 0) {
        free(boletos);
        gerar_erro("Arquivo de remessa invalido: nenhum registro de boleto encontrado apos o cabecalho.");
    }

    // Obtém data do sistema atual para "data de geração" (formato AAAAMMDD)
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char data_geracao[9];
    sprintf(data_geracao, "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    // Regra 2: Gera os arquivos RETORNO agrupando todos os boletos por cliente
    for (int i = 0; i < total_boletos; i++) {
        if (boletos[i].processado) continue;

        char cliente_atual[6];
        strcpy(cliente_atual, boletos[i].cliente);

        char nome_arquivo[50];
        sprintf(nome_arquivo, "%s_%s.txt", data_geracao, cliente_atual);

        FILE *f_out = fopen(nome_arquivo, "w");
        if (!f_out) {
            free(boletos);
            gerar_erro("Falha ao criar o arquivo de retorno no diretorio atual.");
        }

        // Grava o Cabeçalho de Retorno (Tamanho 9)
        fprintf(f_out, "1%s\n", data_geracao);

        int cont_boletos_cliente = 0;
        long long soma_valor_cliente = 0;

        // Varre a lista de boletos carregados vinculando ao cliente atual
        for (int j = i; j < total_boletos; j++) {
            if (!boletos[j].processado && strcmp(boletos[j].cliente, cliente_atual) == 0) {

                // Grava o Boleto Pago (Tamanho 31)
                fprintf(f_out, "2%s%s%s%s\n", 
                        boletos[j].conta, 
                        boletos[j].num_boleto, 
                        boletos[j].data_pagamento, 
                        boletos[j].valor_pago_str);

                cont_boletos_cliente++;
                soma_valor_cliente += boletos[j].valor_pago;
                boletos[j].processado = 1; // Marca como lido para não repetir
            }
        }

        // Grava a Última linha (Rodapé) - %06d e %010lld garantem os zeros à esquerda (Tamanho 17)
        fprintf(f_out, "3%06d%010lld\n", cont_boletos_cliente, soma_valor_cliente);
        fclose(f_out);
    }

    free(boletos);
    return 0;
}
