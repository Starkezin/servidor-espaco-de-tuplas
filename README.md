# Servidor de Espaço de Tuplas (Linda) - Implementação em Elixir

Este projeto consiste na implementação de um servidor de espaço de tuplas (Tuple Space) baseado no modelo **Linda**, desenvolvido para a disciplina de **Programação Concorrente**.

A solução foi construída utilizando **Elixir/OTP**, aproveitando o modelo de atores (Actor Model) para gerenciar concorrência, estado e tolerância a falhas de forma nativa e eficiente.

## Requisitos

* **Elixir 1.14** ou superior.
* **Erlang/OTP 25** ou superior.
* Ferramenta de build **Mix** (incluída no Elixir).

## Compilação e Execução

Para compilar e rodar o servidor, execute os comandos abaixo na raiz do projeto:

1. **Baixar dependências e compilar:**
   ```bash
   mix deps.get
   mix compile
   ```
2. **Iniciar o servidor:**
    ```bash
    iex -S mix
    ```
O servidor iniciará automaticamente e exibirá a mensagem: [info] Servidor ouvindo na porta 54321.

(Para encerrar, pressione Ctrl+C duas vezes).

## Configuração

* **Porta TCP:** 54321.
* **Host:** 127.0.0.1 (Aceita conexões locais e remotas).

### Estrutura de Arquivos

* `lib/linda_server/tuple_space.ex`: GenServer responsável pelo estado (tuplas).
* `lib/linda_server/tcp_listener.ex`: Gerenciador de conexões TCP.
* `lib/linda_server/services.ex`: Lógica dos serviços transformadores (EX).

## Protocolo de Comunicação

O servidor utiliza um protocolo textual via TCP. Os comandos devem ser terminados por quebra de linha (`\n`).

### Operações Suportadas

| Comando | Descrição | Bloqueante? | Resposta Sucesso |
| :--- | :--- | :---: | :--- |
| `WR chave valor` | Escreve uma tupla `(chave, valor)`. | Não | `OK` |
| `RD chave` | Lê o valor de uma tupla sem remover. | **Sim** | `OK valor` |
| `IN chave` | Lê e remove a tupla do espaço. | **Sim** | `OK valor` |
| `EX k_in k_out id` | Consome `k_in`, aplica serviço `id`, escreve em `k_out`. | **Sim** | `OK` |

> **Observação:** Se o serviço solicitado no comando `EX` não existir, o servidor retorna `NO-SERVICE`.

---

## Serviços Implementados (Operação EX)

Conforme a especificação do projeto, os seguintes serviços de transformação de strings estão disponíveis:

| ID (`svc_id`) | Serviço | Descrição | Exemplo |
| :---: | :--- | :--- | :--- |
| **1** | **To Upper** | Converte a string para maiúsculas. | `"ola"` → `"OLA"` |
| **2** | **Reverse** | Inverte a ordem dos caracteres. | `"abc"` → `"cba"` |
| **3** | **Length** | Retorna o tamanho da string. | `"teste"` → `"5"` |