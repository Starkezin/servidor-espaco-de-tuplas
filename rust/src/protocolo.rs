//INTERPRETA OS COMANDOS QUE CHEGAM PELA REDE E CHAMA O QUE FOR PRECISO

use std::sync::Arc;
use crate::espaco_tuplas::EspacoDeTuplas;
use crate::servicos;

pub fn processar(linha: &str, espaco: &Arc<EspacoDeTuplas>) -> String {
    match linha.split_whitespace().collect::<Vec<_>>().as_slice() {
        ["WR", chave, valor] => {
            espaco.escrever(chave.to_string(), valor.to_string());
            "OK".to_string()
        }
        //RD e IN são idênticos, só que IN remove o valor
        ["RD", chave] => format!("OK {}", espaco.ler(chave)),
        ["IN", chave] => format!("OK {}", espaco.consumir(chave)),

        ["EX", entrada, saida, id] => {
            //primeiro consome a tupla, depois verifica se o espaço existe
            let valor = espaco.consumir(entrada);
            match id.parse().ok().and_then(servicos::buscar) {
                Some(servico) => {
                    espaco.escrever(saida.to_string(), servico(&valor));
                    "OK".to_string()
                }
                None => "NO-SERVICE".to_string(), //se o serviço não existe, não insere nada
            }
        }
        _ => "ERROR".to_string(), //qualquer coisa fora do protocolo vira erro
    }
}