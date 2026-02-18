#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

// Envia uma linha (cmd + '\n') e lê uma linha de resposta
std::string send_command(int sock, const std::string &cmd)
{
    std::string to_send = cmd + "\n";
    ssize_t total_sent = 0;
    while (total_sent < static_cast<ssize_t>(to_send.size()))
    {
        ssize_t sent = ::send(sock, to_send.data() + total_sent, to_send.size() - total_sent, 0);
        if (sent <= 0)
        {
            throw std::runtime_error("Erro ao enviar comando ao servidor");
        }
        total_sent += sent;
    }
    std::string response;
    char ch;
    while (true)
    {
        ssize_t rec = ::recv(sock, &ch, 1, 0);
        if (rec <= 0)
        {
            throw std::runtime_error("Conexao encerrada pelo servidor");
        }
        if (ch == '\n')
            break;
        if (ch != '\r')
            response.push_back(ch);
    }
    return response;
}

void expect_prefix(const std::string &resp, const std::string &prefix, const std::string &context)
{
    if (resp.rfind(prefix, 0) != 0)
    {
        std::cerr << "[FALHA] " << context << " resposta inesperada: \"" << resp << "\"\n";
    }
    else
    {
        std::cout << "[OK] " << context << " resposta: \"" << resp << "\"\n";
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Uso: " << argv[0] << " <host> <porta>\n";
        return 1;
    }
    std::string host = argv[1];
    std::string port = argv[2];

    addrinfo hints{}, *result;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int ret = ::getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (ret != 0)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(ret) << "\n";
        return 1;
    }

    int sock = -1;
    for (addrinfo *rp = result; rp != nullptr; rp = rp->ai_next)
    {
        sock = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1)
            continue;
        if (::connect(sock, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        ::close(sock);
        sock = -1;
    }
    freeaddrinfo(result);

    if (sock == -1)
    {
        std::cerr << "Nao foi possivel conectar ao servidor\n";
        return 1;
    }

    try
    {
        std::cout << "Conectado a " << host << ":" << port << "\n";

        // 1) Teste básico de WR e RD
        std::string cmd = "WR teste1 valor1";
        std::string resp = send_command(sock, cmd);
        expect_prefix(resp, "OK", "WR teste1");

        cmd = "RD teste1";
        resp = send_command(sock, cmd);
        expect_prefix(resp, "OK", "RD teste1");
        std::cout << " (RD teste1 retornou: \"" << resp << "\")\n";

        // 2) Teste de IN removendo a tupla
        cmd = "IN teste1";
        resp = send_command(sock, cmd);
        expect_prefix(resp, "OK", "IN teste1");

        // 3) Teste de EX com svc_id = 1
        cmd = "WR in1 abcdef";
        send_command(sock, cmd);
        cmd = "EX in1 out1 1";
        resp = send_command(sock, cmd);
        expect_prefix(resp, "OK", "EX 1");

        cmd = "RD out1";
        resp = send_command(sock, cmd);
        expect_prefix(resp, "OK", "RD out1 apos EX 1");

        // 4) Teste de EX com svc_id = 2
        cmd = "WR in2 ghijkl";
        send_command(sock, cmd);
        cmd = "EX in2 out2 2";
        resp = send_command(sock, cmd);
        expect_prefix(resp, "OK", "EX 2");

        // 5) Teste de EX com serviço inexistente
        cmd = "WR in3 xyz";
        send_command(sock, cmd);
        cmd = "EX in3 out3 99";
        resp = send_command(sock, cmd);
        expect_prefix(resp, "NO-SERVICE", "EX 99");

        std::cout << "Testes basicos concluidos.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Erro: " << e.what() << "\n";
        ::close(sock);
        return 1;
    }
    ::close(sock);
    return 0;
}