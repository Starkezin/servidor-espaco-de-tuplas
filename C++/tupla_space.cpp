#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <queue>
#include <sstream>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

// Classe do Espaço de Tuplas
class TupleSpace
{
private:
    unordered_map<string, queue<string>> space;
    mutex mtx;
    condition_variable cv;

public:
    // WR: Insere a tupla e acorda quem está esperando
    void wr(const string &key, const string &value)
    {
        unique_lock<mutex> lock(mtx);
        space[key].push(value);
        cv.notify_all();
    }

    // RD: Lê sem remover, bloqueia se não existir
    string rd(const string &key)
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this, &key]()
                { return !space[key].empty(); });
        return space[key].front();
    }

    // IN: Lê e remove, bloqueia se não existir
    string in(const string &key)
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this, &key]()
                { return !space[key].empty(); });
        string value = space[key].front();
        space[key].pop();
        return value;
    }
};

// Função simples para rotear os serviços
bool executar_servico(const string &id, const string &input, string &output)
{
    if (id == "1")
    {
        output = input;
        for (char &c : output)
        {
            c = toupper(c);
        }
        return true;
    }
    else if (id == "2")
    {
        output = input;
        reverse(output.begin(), output.end());
        return true;
    }
    else if (id == "3")
    {
        output = to_string(input.length());
        return true;
    }
    return false; // Retorna falso se o serviço não existir
}

// Facilita o envio de respostas pela rede
void enviar_resposta(int sock, const string &msg)
{
    string pacote = msg + "\n";
    send(sock, pacote.c_str(), pacote.length(), 0);
}

// Função executada por cada thread (cada cliente conectado)
void handle_client(int client_sock, TupleSpace *ts)
{
    char ch;
    string linha = "";

    while (true)
    {
        ssize_t bytes_lidos = recv(client_sock, &ch, 1, 0);
        if (bytes_lidos <= 0)
            break; // Sai se o cliente fechar a conexão

        if (ch == '\n')
        {
            istringstream iss(linha);
            string comando;
            iss >> comando;

            if (comando == "WR")
            {
                string chave, valor;
                iss >> chave;
                getline(iss >> ws, valor); // Pega o resto da linha como valor
                ts->wr(chave, valor);
                enviar_resposta(client_sock, "OK");
            }
            else if (comando == "RD")
            {
                string chave;
                iss >> chave;
                string valor = ts->rd(chave);
                enviar_resposta(client_sock, "OK " + valor);
            }
            else if (comando == "IN")
            {
                string chave;
                iss >> chave;
                string valor = ts->in(chave);
                enviar_resposta(client_sock, "OK " + valor);
            }
            else if (comando == "EX")
            {
                string key_in, key_out, svc_id;
                iss >> key_in >> key_out >> svc_id;

                // Bloqueia até a tupla de entrada existir e a remove
                string val_in = ts->in(key_in);
                string val_out;

                // Tenta executar o serviço
                if (executar_servico(svc_id, val_in, val_out))
                {
                    ts->wr(key_out, val_out);
                    enviar_resposta(client_sock, "OK");
                }
                else
                {
                    enviar_resposta(client_sock, "NO-SERVICE");
                }
            }
            else
            {
                enviar_resposta(client_sock, "ERROR");
            }
            linha.clear(); // Prepara para o próximo comando
        }
        else if (ch != '\r')
        {
            linha += ch;
        }
    }
    close(client_sock);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Uso: " << argv[0] << " <porta>\n";
        return 1;
    }

    int port = stoi(argv[1]);
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock < 0)
    {
        cerr << "Erro ao criar socket\n";
        return 1;
    }

    // Configuração para evitar erro de "porta em uso" ao reiniciar rapidamente
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "Erro no bind da porta " << port << "\n";
        return 1;
    }

    listen(server_sock, 10);

    TupleSpace ts;
    cout << "Servidor Linda inicializado na porta " << port << "...\n";

    // Loop principal aceitando clientes
    while (true)
    {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

        if (client_sock >= 0)
        {
            // Cria uma thread para o cliente e passa o ponteiro do TupleSpace
            thread(handle_client, client_sock, &ts).detach();
        }
    }

    close(server_sock);
    return 0;
}