defmodule LindaServer.Services do
  def execute(svc_id, value) do
    case svc_id do
      "1" -> {:ok, String.upcase(value)}
      "2" -> {:ok, String.reverse(value)}
      "3" -> {:ok, Integer.to_string(String.length(value))}
      _ -> :error
    end
  end
end
