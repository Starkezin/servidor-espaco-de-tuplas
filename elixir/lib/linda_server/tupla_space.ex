defmodule LindaServer.TupleSpace do
  use GenServer
  require Logger

  # Estrutura do Estado
  defstruct tuples: [], waiters: []

  # --- API Pública ---

  def start_link(_) do
    GenServer.start_link(__MODULE__, %__MODULE__{}, name: __MODULE__)
  end

  def write(key, value) do
    GenServer.cast(__MODULE__, {:write, key, value})
  end

  def read(key) do
    GenServer.call(__MODULE__, {:read, key}, :infinity)
  end

  def in_op(key) do
    GenServer.call(__MODULE__, {:in, key}, :infinity)
  end

  # --- Callbacks do GenServer ---

  @impl true
  def init(state) do
    Logger.info("Espaço de Tuplas iniciado.")
    {:ok, state}
  end

  # Operação WR: Nunca bloqueia
  @impl true
  def handle_cast({:write, key, value}, state) do
    new_tuple = {key, value}
    # Tenta casar a nova tupla com alguém esperando
    new_state = match_tuple_to_waiters(new_tuple, state)
    {:noreply, new_state}
  end

  # Operação RD: Bloqueia se não achar
  @impl true
  def handle_call({:read, key}, from, state) do
    # Procura a tupla mais antiga (FIFO)
    case find_tuple(state.tuples, key) do
      {:ok, value} ->
        {:reply, {:ok, value}, state}
      nil ->
        # Não achou: Adiciona à lista de espera e NÃO responde (bloqueia o cliente)
        waiter = {:rd, key, from}
        {:noreply, %{state | waiters: state.waiters ++ [waiter]}}
    end
  end

  # Operação IN: Bloqueia se não achar e remove
  @impl true
  def handle_call({:in, key}, from, state) do
    case find_and_remove_tuple(state.tuples, key) do
      {:ok, value, new_tuples} ->
        {:reply, {:ok, value}, %{state | tuples: new_tuples}}
      nil ->
        # Não achou: Bloqueia
        waiter = {:in, key, from}
        {:noreply, %{state | waiters: state.waiters ++ [waiter]}}
    end
  end


  # Encontra a primeira tupla compatível (FIFO)
  defp find_tuple(tuples, key) do
    case Enum.find(tuples, fn {k, _v} -> k == key end) do
      {_, val} -> {:ok, val}
      nil -> nil
    end
  end

  # Encontra e remove a primeira tupla (FIFO)
  defp find_and_remove_tuple(tuples, key) do
    index = Enum.find_index(tuples, fn {k, _v} -> k == key end)
    if index do
      {{_k, val}, new_tuples} = List.pop_at(tuples, index)
      {:ok, val, new_tuples}
    else
      nil
    end
  end

  # Lógica principal de Matching: Chegou uma tupla, quem quer ela?
  defp match_tuple_to_waiters({key, value} = tuple, state) do
    {matches, remaining_waiters} = Enum.split_with(state.waiters, fn {_, k, _} -> k == key end)

    # Processa os waiters em ordem FIFO
    {remaining_tuple, final_waiters} =
      Enum.reduce_while(matches, {true, []}, fn {type, _k, from}, {tuple_exists, acc_waiters} ->
        cond do
          # Se a tupla já foi consumida por um IN anterior neste loop, o waiter continua esperando
          not tuple_exists ->
            {:cont, {false, acc_waiters ++ [{type, key, from}]}}

          # Se for RD, responde o cliente, mas a tupla continua existindo
          type == :rd ->
            GenServer.reply(from, {:ok, value})
            {:cont, {true, acc_waiters}} # Tupla mantida, waiter removido da lista

          # Se for IN, responde o cliente, a tupla é destruída e paramos de procurar waiters
          type == :in ->
            GenServer.reply(from, {:ok, value})
            {:halt, {false, acc_waiters}} # Tupla destruída, loop encerrado
        end
      end)

    new_waiters_list = remaining_waiters ++ final_waiters

    new_tuples_list = if remaining_tuple, do: state.tuples ++ [tuple], else: state.tuples

    %{state | tuples: new_tuples_list, waiters: new_waiters_list}
  end
end
