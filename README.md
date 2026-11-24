# Gerador de arquivos de retorno de boletos

Projeto final da disciplina de **Programação Estruturada**. O programa foi
desenvolvido em linguagem C e processa um arquivo de remessa com boletos pagos,
validando os dados e gerando um arquivo de retorno separado para cada cliente.

O sistema recebe o caminho do arquivo pela linha de comando, lê os registros em
formato posicional, soma os valores em centavos e grava os arquivos no diretório
atual.

## Funcionalidades

- Leitura de arquivo de remessa por linha de comando.
- Suporte a linha de cabeçalho com identificação da data.
- Validação dos identificadores, campos numéricos e datas.
- Validação do tamanho das linhas.
- Suporte a BOM UTF-8 no início do cabeçalho.
- Agrupamento dos boletos por código de cliente.
- Geração de um arquivo de retorno por cliente.
- Soma dos valores sem conversão de ponto flutuante.
- Limite de quantidade e valor conforme o tamanho dos campos de retorno.
- Registro de falhas no arquivo `ERRO.TXT`.
- Liberação da memória alocada dinamicamente ao finalizar.

## Tecnologias

- Linguagem C
- Biblioteca padrão C
- Compilador GCC
- Arquivos texto com formato posicional

## Estrutura do projeto

```text
projeto_faculdade/
├── projeto_final.c       Código-fonte principal
├── projeto_final.exe     Executável já compilado no Windows
├── remessa.txt           Exemplo de arquivo de remessa
├── 20261124_00010.txt    Exemplos de arquivos de retorno
├── 20261124_00015.txt
├── 20261124_00100.txt
├── 20261124_00110.txt
├── 20261124_01170.txt
├── 20261124_03110.txt
└── README.md             Documentação do projeto
```

## Formato do arquivo de remessa

O arquivo deve conter uma linha de cabeçalho seguida por uma ou mais linhas de
boletos pagos.

### Cabeçalho

O cabeçalho possui exatamente 9 caracteres.

| Posição | Tamanho | Conteúdo |
|---|---:|---|
| 1 | 1 | Identificador do registro: `1` |
| 2 a 9 | 8 | Data da remessa no formato `AAAAMMDD` |

Exemplo:

```text
120260614
```

Nesse exemplo, a data da remessa é `20260614`.

### Registro de boleto pago

Cada boleto possui exatamente 44 caracteres.

| Posição | Tamanho | Conteúdo |
|---|---:|---|
| 1 | 1 | Identificador do registro: `2` |
| 2 a 6 | 5 | Código do cliente |
| 7 a 12 | 6 | Número da conta |
| 13 a 20 | 8 | Número do boleto |
| 21 a 28 | 8 | Data de vencimento em `AAAAMMDD` |
| 29 a 36 | 8 | Data de pagamento em `AAAAMMDD` |
| 37 a 44 | 8 | Valor pago em centavos |

Exemplo:

```text
20001041988687433457202603172026070400013265
```

Os campos possuem a seguinte interpretação:

- Cliente: `00010`
- Conta: `419886`
- Boleto: `87433457`
- Vencimento: `20260317`
- Pagamento: `20260704`
- Valor: `00013265`

> O valor informado no campo `VALOR_PAGO` não contém separador decimal. Por
> exemplo, `00001027` representa 10 reais e 27 centavos.

Não são permitidas linhas em branco antes do final do arquivo. Linhas em branco
no meio da remessa são consideradas inválidas.

## Formato do arquivo de retorno

O nome de cada arquivo de retorno segue o padrão:

```text
AAAAMMDD_CODIGO_DO_CLIENTE.txt
```

`AAAAMMDD` é a data atual da máquina no momento da execução. Por exemplo:

```text
20261124_00100.txt
```

Cada arquivo possui três tipos de linha:

### Cabeçalho

| Posição | Tamanho | Conteúdo |
|---|---:|---|
| 1 | 1 | Identificador do registro: `1` |
| 2 a 9 | 8 | Data de geração em `AAAAMMDD` |

Exemplo:

```text
120261124
```

### Detalhe do boleto

| Posição | Tamanho | Conteúdo |
|---|---:|---|
| 1 | 1 | Identificador do registro: `2` |
| 2 a 7 | 6 | Número da conta |
| 8 a 15 | 8 | Número do boleto |
| 16 a 23 | 8 | Data de pagamento |
| 24 a 31 | 8 | Valor pago em centavos |

Exemplo:

```text
2687433459202602026042700010276
```

### Rodapé

| Posição | Tamanho | Conteúdo |
|---|---:|---|
| 1 | 1 | Identificador do registro: `3` |
| 2 a 7 | 6 | Quantidade de boletos |
| 8 a 17 | 10 | Valor total em centavos |

Exemplo:

```text
300000100000010276
```

No rodapé, a quantidade é preenchida com zeros à esquerda e o valor total é
preenchido com dez dígitos.

## Como compilar

É necessário um compilador C compatível com a biblioteca padrão. Com o GCC:

```bash
gcc projeto_final.c -o projeto_final.exe
```

Em sistemas Unix/Linux, o executável pode usar outro nome:

```bash
gcc projeto_final.c -o projeto_final
```

## Como executar

No Windows:

```powershell
.\projeto_final.exe remessa.txt
```

No Linux:

```bash
./projeto_final arquivo_de_remessa.txt
```

Ao executar com sucesso, o programa informa no terminal quantos boletos foram
processados e quantos arquivos de retorno foram criados:

```text
Processamento concluido: 3501 boleto(s) lido(s), 7 arquivo(s) de retorno gerado(s).
```

Os arquivos gerados são escritos no diretório de onde o programa foi executado.

## Validações realizadas

O programa rejeita o arquivo de remessa quando:

- nenhum argumento é informado;
- são informados argumentos em excesso;
- o arquivo não pode ser aberto;
- o arquivo está vazio;
- não existe nenhum boleto após o cabeçalho;
- o cabeçalho não começa com `1`;
- o cabeçalho não possui 9 caracteres;
- a data do cabeçalho é inválida;
- um registro de boleto não começa com `2`;
- um boleto não possui 44 caracteres;
- códigos de cliente, conta, boleto ou valor não são numéricos;
- a data de vencimento ou pagamento é inválida;
- existem linhas em branco no meio do arquivo;
- a quantidade de boletos de um cliente excede 999.999;
- a soma dos valores de um cliente excede 9.999.999.999 centavos;
- não é possível criar um arquivo de retorno.

Datas válidas devem estar no formato `AAAAMMDD`, entre os anos de 1900 e 2100,
e devem respeitar os meses de cada ano e os anos bissextos.

## Arquivo de erro

Em caso de validação incorreta, o programa encerra a execução com código de saída
`1`, escreve a mensagem no arquivo `ERRO.TXT` e também mostra o erro no terminal.

Exemplo:

```text
ERRO ENCONTRADO NO ARQUIVO DE REMESSA:
Linha 2 com tamanho incorreto: esperado 44 caracteres, encontrado 43.
```

Quando um processamento é concluído, um `ERRO.TXT` existente é removido.

## Organização do código

O código está organizado nas seguintes partes:

- constantes e limites dos campos;
- estrutura `Boleto`, que guarda os dados já separados;
- `gerar_erro`, responsável pelo encerramento e geração do arquivo de erro;
- `limpar_quebra_linha`, `eh_numerico` e `data_valida`, usados nas validações;
- `copiar_campo` e `ler_boleto`, responsáveis pela leitura posicional;
- `totalizar_cliente`, usada para contar e somar registros;
- `primeira_ocorrencia`, usada para evitar gerar arquivos repetidos;
- `gerar_retorno`, que cria o cabeçalho, detalhes e rodapé;
- `main`, que controla o fluxo completo do programa.
