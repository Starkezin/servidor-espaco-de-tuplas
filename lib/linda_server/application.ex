defmodule LindaServer.Application do
  use Application

  @impl true
  def start(_type, _args) do
    children = [
      LindaServer.TupleSpace,   # Inicia o Banco de Dados
      LindaServer.TcpListener   # Inicia o Servidor TCP
    ]

    opts = [strategy: :one_for_one, name: LindaServer.Supervisor]
    Supervisor.start_link(children, opts)
  end
end
