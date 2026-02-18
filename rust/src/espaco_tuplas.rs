//LÓGICA DO ESPAÇO DE TUPLAS

use std::collections::{HashMap, VecDeque};
use std::sync::{Condvar, Mutex};

//cada chave tem uma fita de valores, suportando várias tuplas com a mesma chave
#[derive(Default)]
pub struct EspacoDeTuplas {
    mapa: Mutex<HashMap<String, VecDeque<String>>>,
    cond: Condvar, //serve pra acordar threads que esperam uma tupla aparecer
}

impl EspacoDeTuplas {
    //inserção do valor no final da fila da chave e avisa quem tá esperando
    pub fn escrever(&self, chave: String, valor: String) {
        self.mapa.lock().unwrap().entry(chave).or_default().push_back(valor);
        self.cond.notify_all();
    }

    //lê o primeiro valor sem removê-lo. se não tiver nada, dorme e espera
    pub fn ler(&self, chave: &str) -> String {
        let mut mapa = self.mapa.lock().unwrap();
        loop {
            if let Some(valor) = mapa.get(chave).and_then(|f| f.front()) {
                return valor.clone();
            }
            mapa = self.cond.wait(mapa).unwrap();
        }
    }

    //igual ao ler, mas remove o valor da fila depois
    pub fn consumir(&self, chave: &str) -> String {
        let mut mapa = self.mapa.lock().unwrap();
        loop {
            if let Some(valor) = mapa.get_mut(chave).and_then(|f| f.pop_front()) {
                return valor;
            }
            mapa = self.cond.wait(mapa).unwrap();
        }
    }
}