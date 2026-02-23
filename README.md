# Servidor de Espaço de Tuplas (Linda) - Multi-Linguagens

Implementação de um servidor concorrente de **Espaço de Tuplas** (Tuple Space) inspirado no modelo **Linda**, desenvolvido em múltiplas linguagens de programação como projeto educacional.

Participantes do grupo:
- Lucas Ludwig
- Samuel Starke
- Sandro Júnior 
- Vítor COlombo

---

## Estrutura do Projeto

```
linda_server/
├── README.md                    # Este arquivo
├── tester_linda.cpp             # Cliente de teste (c++)
├── C++/
│   ├── tester_linda.cpp        # Cliente de teste (C++)
│   └── tupla_space.cpp         # Servidor (C++)
│   
├── Go/
│   ├── main.go                 # Servidor (Go)
│   ├── go.mod                  # Dependências
│   └── README.md               # Documentação específica
├── rust/
│   ├── src/
│   │   ├── main.rs
│   │   ├── espaco_tuplas.rs
│   │   ├── protocolo.rs
│   │   ├── servicos.rs
│   │   └── servidor.rs
│   ├── Cargo.toml              # Dependências
│   ├── Cargo.lock
│   └── README.md               # Documentação específica
└── elixir/
    ├── lib/
    │   ├── linda_server.ex
    │   └── linda_server/
    │       ├── application.ex
    │       ├── services.ex
    │       ├── tcp_listener.ex
    │       └── tupla_space.ex
    ├── test/
    ├── mix.exs                 # Dependências
    ├── mix.lock
    └── README.md               # Documentação específica
```

---

## Como Rodar os Servidores

### 1. Go

**Compilação:**
```bash
cd Go
go run main.go
```

O servidor iniciará ouvindo na porta **54321**.

**Testar:**
```bash
# Em outro terminal, na raiz do projeto
./tester_linda 127.0.0.1 54321
```

---

### 2. Rust

**Compilação e Execução:**
```bash
cd rust
cargo run
```

O servidor iniciará ouvindo na porta **54321**.

**Testar:**
```bash
# Em outro terminal, na raiz do projeto
./tester_linda 127.0.0.1 54321
```

---

### 3. Elixir

**Instalação de dependências:**
```bash
cd elixir
mix deps.get
mix compile
```

**Execução:**
```bash
iex -S mix
```

O servidor iniciará ouvindo na porta **54321** e exibirá a mensagem: `[info] Servidor ouvindo na porta 54321`.

**Testar:**
```bash
# Em outro terminal, na raiz do projeto
./tester_linda 127.0.0.1 54321
```

---

### 4. C++

**Compilação (automática com `make build`):**
```bash
g++ -std=c++17 -pthread C++/tupla_space.cpp -o C++/tupla_space_server
```

**Execução:**
```bash
./C++/tupla_space_server 54321
```

O servidor iniciará ouvindo na porta **54321**.

**Testar:**
```bash
# Em outro terminal, na raiz do projeto
./tester_linda 127.0.0.1 54321
```

---

## Requisitos

| Linguagem | Versão Mínima |
| :--- | :--- |
| **Go** | 1.18+ |
| **Rust** | Última versão estável |
| **Elixir** | 1.14+ (requer Erlang/OTP 25+) |
| **C++** | C++17 (compilador g++) |

---

## Protocolo de Comunicação

Todos os servidores implementam o mesmo protocolo textual sobre TCP:

| Comando | Descrição | Bloqueante? | Resposta |
| :--- | :--- | :---: | :--- |
| **`WR chave valor`** | Escreve tupla `(chave, valor)` | Não | `OK` |
| **`RD chave`** | Lê sem remover | **Sim** | `OK valor` |
| **`RMV chave`** | Remove tupla | **Sim** | `OK valor` |
| **`EX chave servico`** | Executa serviço transformador | Sim | `OK resultado` |

### Operações Bloqueantes

- **RD** e **RMV** bloqueiam se a chave não existir
- Retornam quando a tupla estiver disponível
- Podem ser desbloqueadas por outro cliente escrevendo a tupla

---


## Exemplo de Uso

Após iniciar um servidor, use o cliente de teste:

```bash
./tester_linda 127.0.0.1 54321
```

A interface permite executar operações:

```
WR nome Alice
OK
RD nome
OK Alice
RMV idade
(Aguarda até alguém escrever 'idade')
```

---
