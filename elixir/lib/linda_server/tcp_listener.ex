defmodule LindaServer.TcpListener do
  require Logger

  # Porta padrão definida na especificação
  @port 54321

  # --- CORREÇÃO DO CRASH ---
  # Esta função ensina ao Supervisor como iniciar este processo.
  def child_spec(opts) do
    %{
      id: __MODULE__,
      start: {__MODULE__, :start_link, [opts]},
      type: :worker,
      restart: :permanent,
      shutdown: 500
    }
  end

  def start_link(_) do
    # Inicia o loop de escuta em uma Task supervisionada
    Task.start_link(fn -> listen() end)
  end

  def listen do
    opts = [:binary, packet: :line, active: false, reuseaddr: true]
    case :gen_tcp.listen(@port, opts) do
      {:ok, socket} ->
        Logger.info("Servidor ouvindo na porta #{@port}")
        accept_loop(socket)
      {:error, reason} ->
        Logger.error("Falha ao abrir porta: #{inspect(reason)}")
    end
  end

  defp accept_loop(socket) do
    {:ok, client} = :gen_tcp.accept(socket)
    # Spawna um processo para cada cliente (concorrência)
    Task.start(fn -> handle_client(client) end)
    accept_loop(socket)
  end

  defp handle_client(socket) do
    case :gen_tcp.recv(socket, 0) do
      {:ok, data} ->
        command = String.trim(data)
        Logger.info("Comando recebido: #{inspect(command)}")

        try do
          response = process_command(command)
          Logger.info("Resposta: #{inspect(response)}")
          :gen_tcp.send(socket, response <> "\n")
        rescue
          e ->
            Logger.error("Erro ao processar comando: #{inspect(e)}")
            :gen_tcp.send(socket, "ERROR\n")
        end

        handle_client(socket) # Loop para manter conexão viva
      {:error, :closed} ->
        Logger.info("Cliente desconectado")
        :ok
      {:error, reason} ->
        Logger.error("Erro na conexão: #{inspect(reason)}")
        :ok
    end
  end

  # --- Parsing e Execução de Comandos ---

  defp process_command(line) do
    parts = String.split(line, " ", parts: 4)

    case parts do
      ["WR", _k, _v] ->
        [_cmd, key | val_parts] = String.split(line, " ")
        value = Enum.join(val_parts, " ")
        LindaServer.TupleSpace.write(key, value)
        "OK"

      ["RD", key] ->
        {:ok, val} = LindaServer.TupleSpace.read(key)
        "OK #{val}"

      ["IN", key] ->
        {:ok, val} = LindaServer.TupleSpace.in_op(key)
        "OK #{val}"

      ["EX", k_in, k_out, svc_id] ->
        handle_ex(k_in, k_out, svc_id)

      _ ->
        "ERROR"
    end
  end

  defp handle_ex(k_in, k_out, svc_id) do
    if svc_id not in ["1", "2", "3"] do
      "NO-SERVICE"
    else
      {:ok, v_in} = LindaServer.TupleSpace.in_op(k_in)

      case LindaServer.Services.execute(svc_id, v_in) do
        {:ok, v_out} ->
          LindaServer.TupleSpace.write(k_out, v_out)
          "OK"
        :error ->
          "NO-SERVICE"
      end
    end
  end
end
