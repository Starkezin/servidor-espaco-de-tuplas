//ABRE A PORTA E GERENCIA AS CONEXÕES DOS CLIENTES

use std::io::{BufRead, BufReader, Write};
use std::net::TcpListener;
use std::sync::Arc;
use std::thread;

use crate::espaco_tuplas::EspacoDeTuplas;

pub fn iniciar(espaco: Arc<EspacoDeTuplas>) {
    let ouvinte = TcpListener::bind("0.0.0.0:54321").unwrap();
    println!("[SERVIDOR ESCUTANDO NA PORTA 54321]");

    for conexao in ouvinte.incoming().flatten() {
    let espaco = Arc::clone(&espaco);

        //cada cliente recebe sua thread própria pra não travar outros clientes
        thread::spawn(move || {
            let mut fluxo = conexao;
            for linha in BufReader::new(fluxo.try_clone().unwrap()).lines().flatten() {
                let resposta = crate::protocolo::processar(&linha, &espaco);
                if writeln!(fluxo, "{}", resposta).is_err() { 
                    break; //cliente desconectou -> encerra a thread
                }
            }
        });
    }
}