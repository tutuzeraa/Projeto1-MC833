# Projeto1-MC833
Projeto 1 de Lab de Redes - Sistema de Streaming de Filmes usando TCP

Sistema concorrente para consulta de filmes. Arquitetura cliente-servidor usando sockets TCP.

Stack: 

-> Linguagem:    C

-> Database:     SQLite

-> Concorrência: pthreads


_Disclaimer:_ Código e comentários foram escritos em Inglês

## Como rodar
Requisitos:
- libsqlite3-dev (sqlite3)
- libpthread-stubs0-dev (pthreads)

```bash
git clone git@github.com:tutuzeraa/Projeto1-MC833.git
// é necessário mudar em ./src/client.c o SERVER_ADDR para o IP da rede do servidor
cd Projeto1-MC833/build
make 
./server
./client
```