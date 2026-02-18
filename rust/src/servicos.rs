//TABELA COM OS SERVIÇOS QUE O SERVIDOR SABE EXECUTAR

pub type FuncaoServico = fn(&str) -> String;

//tabela estática de serviços (id, função)
pub const SERVICOS: &[(u32, FuncaoServico)] = &[
    (1, |s| s.to_uppercase()),               //converte tudo pra maiúsculas
    (2, |s| s.chars().rev().collect()),       //inverte a string
    (3, |s| s.len().to_string()),             //retorna o tamanho como texto
];

//procura o serviço pela função, retorna NONE se não existir
pub fn buscar(id: u32) -> Option<FuncaoServico> {
    SERVICOS.iter().find(|(i, _)| *i == id).map(|(_, f)| *f)
}