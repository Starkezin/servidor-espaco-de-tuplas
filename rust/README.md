# Servidor de Espaço de Tuplas (Linda) — Rust

Implementação de um servidor concorrente de espaço de tuplas inspirado no modelo Linda, desenvolvida em Rust como projeto final da disciplina de Programação Concorrente.

A solução foi construída aproveitando o modelo de threads nativas com Mutex e Condvar do Rust para gerenciar concorrência e sincronização de forma segura e eficiente, sem o uso de unsafe como descrito na especificação.

---

## Compilação e Execução

**Pré-requisito:** ter o Rust instalado (`https://rustup.rs`).

Dentro da pasta do projeto:

```bash
cargo run
```

O servidor passará a escutar na porta **54321**. Para encerrar a escuta, pressione `Ctrl+C`.

---

## Porta Utilizada

- Porta TCP: 54321;
- End. host: `0.0.0.0` significa que aceita conexões de qualquer interface de rede, incluindo conexões locais via `127.0.0.1`.

---

## Estrutura do Projeto

```
rust/
  Cargo.toml              — configuração e metadados do projeto
  Cargo.lock              — versões fixas das dependências
  tester_linda.cpp        — programa de teste oficial da disciplina
  src/
    main.rs               — ponto de entrada, inicializa e liga as peças
    espaco_de_tuplas.rs   — estrutura central com Mutex + Condvar
    protocolo.rs          — parsing e despacho dos comandos TCP
    servicos.rs           — tabela estática de serviços disponíveis
    servidor.rs           — listener TCP, uma thread por cliente
```

---

## Protocolo de Comunicação

O servidor se comunica via TCP com comandos em texto puro, um por linha, terminados em `\n`.

**Comandos disponíveis:**

- `WR chave valor` — insere a tupla no espaço. Nunca bloqueia. Retorna `OK`.
- `RD chave` — lê o valor sem remover. Bloqueia se a chave não existir. Retorna `OK valor`.
- `IN chave` — lê e remove o valor. Bloqueia se a chave não existir. Retorna `OK valor`.
- `EX k_entrada k_saida id` — aplica um serviço sobre a tupla e insere o resultado. Retorna `OK` ou `NO-SERVICE`.
- qualquer outro comando — retorna `ERROR`.

---

## Serviços Disponíveis

- `1` — converte para maiúsculas. Exemplo: `hello` → `HELLO`
- `2` — inverte a string. Exemplo: `hello` → `olleh`
- `3` — retorna o tamanho da string. Exemplo: `hello` → `5`

Qualquer `svc_id` não listado retorna `NO-SERVICE`.

---

## Exemplos de Interação via TCP

Conecte ao servidor usando `nc` (netcat):

```bash
nc 127.0.0.1 54321
```

**Escrita e leitura básica:**
```
WR nome joao
OK
RD nome
OK joao
IN nome
OK joao
```

**Múltiplas tuplas na mesma chave (FIFO):**
```
WR cor vermelho
WR cor azul
WR cor verde
IN cor
OK vermelho
IN cor
OK azul
IN cor
OK verde
```

**Aplicando serviços:**
```
WR palavra hello
EX palavra resultado 1
RD resultado
OK HELLO

WR palavra hello
EX palavra resultado 2
RD resultado
OK olleh

WR palavra hello
EX palavra resultado 3
RD resultado
OK 5
```

**Serviço inexistente:**
```
WR entrada hello
EX entrada saida 99
NO-SERVICE
```

**Comportamento bloqueante (dois terminais):**
```
# Terminal A — trava esperando a tupla
RD aguarda

# Terminal B — escreve e desbloqueia o Terminal A automaticamente
WR aguarda apareci
OK
```

---

## Teste com o Programa Teste

Compile e execute o tester:

```bash
g++ -std=c++17 tester_linda.cpp -o tester_linda
./tester_linda 127.0.0.1 54321
```

Saída esperada com todos os testes sendo concluídos:

```
Conectado a 127.0.0.1:54321
[OK] WR teste1 resposta: "OK"
[OK] RD teste1 resposta: "OK valor1"
 (RD teste1 retornou: "OK valor1")
[OK] IN teste1 resposta: "OK valor1"
 (IN teste1 retornou: "OK valor1")
[OK] WR in1 resposta: "OK"
[OK] EX 1 resposta: "OK"
[OK] RD out1 apos EX 1 resposta: "OK ABCDEF"
 (RD out1 apos EX 1 retornou: "OK ABCDEF")
[OK] WR in2 resposta: "OK"
[OK] EX 2 resposta: "OK"
[OK] RD out2 apos EX 2 resposta: "OK LKJIHG"
 (RD out2 apos EX 2 retornou: "OK LKJIHG")
[OK] WR in3 resposta: "OK"
[OK] EX 99 resposta: "NO-SERVICE"
Testes basicos concluidos.
```

---

## Detalhes de Implementação

- **Concorrência:** para cada cliente que conecta é criada uma thread dedicada, permitindo múltiplos clientes simultâneos. O espaço de tuplas é protegido por `Mutex` e as operações bloqueantes usam `Condvar` para suspender a thread sem busy-waiting.

- **Política FIFO:** cada chave mantém uma fila independente. As operações `RD`, `IN` e `EX` sempre acessam o valor mais antigo da fila daquela chave.

- **Sem bibliotecas externas:** toda a implementação usa exclusivamente a biblioteca padrão do Rust (`std`). Nenhum `unsafe` foi utilizado.
