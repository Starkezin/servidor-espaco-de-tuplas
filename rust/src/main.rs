//PONTO DE ENTRADA DO SERVIDOR LINDA

mod espaco_tuplas;
mod servicos;
mod servidor;
mod protocolo;

use std::sync::Arc;
use espaco_tuplas::EspacoDeTuplas;

fn main() {
    servidor::iniciar(Arc::new(EspacoDeTuplas::default())); //cria o espaço compartilhado e passa pro servidor cuidar do resto
}