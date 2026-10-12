# Simulazione di un libro mastro distribuito (Sistemi Operativi)

Progetto in C (C89, Linux/Unix) che simula un sistema di pagamenti in stile blockchain con processi che comunicano tramite IPC System V.

## Come funziona

- **`master`**: crea memoria condivisa, semaforo e coda di messaggi, avvia nodi e utenti con `fork`/`execl` e ogni 5 secondi stampa lo stato (budget degli utenti e contenuto del libro mastro). Dopo `SO_SIM_SEC` cicli, o con `Ctrl+C`, termina tutti i processi e libera le risorse IPC.
- **`utente`**: parte con `SO_BUDGET_INIT` di budget. A intervalli casuali sceglie un altro utente e invia una transazione, con una percentuale `SO_REWARD` destinata al nodo, sulla coda di messaggi.
- **`nodo`**: legge le transazioni dalla coda e le mette nella transaction pool. Quando un blocco è completo lo scrive nel **libro mastro** (memoria condivisa), aggiungendo una transazione di reward.

| Risorsa IPC | Uso |
|---|---|
| Memoria condivisa | libro mastro e PID di utenti e nodi |
| Semaforo | mutua esclusione sull'accesso alla memoria condivisa |
| Coda di messaggi | transazioni da utenti a nodi |

## Compilazione ed esecuzione

```bash
make
./master
```

I tre eseguibili (`master`, `nodo`, `utente`) devono stare nella stessa cartella. I log vanno in `log_user.txt`, `log_node.txt` e nei relativi `*_err.txt`. `make clean` rimuove eseguibili e log.

## Parametri

Si modificano in `lib.h`: `SO_USERS_NUM`, `SO_NODES_NUM`, `SO_BLOCK_SIZE`, `SO_REGISTRY_SIZE`, `SO_TP_SIZE`, `SO_BUDGET_INIT`, `SO_REWARD`, `SO_SIM_SEC`, ecc.

## File

`master.c`, `nodo.c`, `utente.c`, `lib.h` (costanti e strutture condivise), `Makefile`.
