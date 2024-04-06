# Shell Didattica (dsh)

## Requisiti

- Nome del file esguibile: `dsh`
- Nome del file sorgente: `dsh.c` (`gcc dsh.c -o dsh`)

---

1. Presentare il prompt `dsh$ ` all'apertura (se non vengono specificati parametri)
2. Interpretare i comandi dell'utente inseriti da riga di comando
   - Built-in (`exit`, `setpath`)
     - Possibilitá di specificare il PATH tramite una stringa della forma `/bin/:/usr/bin/` ecc.
     - Il built-in `setpath` deve restituire all'utente il path attuale se non viene specificato nulla
     - Devo poter uscire dalla shell anche con Ctrl-C e Ctrl-D
   - I comandi generici della forma `ls -la` e `/bin/wc -l`
   - Funzioni speciali: Redirezione `>`, `|`, `&`, `>>`, `;`
3. Interpretare i comandi dell'utente caricandoli da file di script
   - File `pippo.dsh` che carico ed eseguo riga per riga
   - Passato come argomento a riga di comando
   - Con un built-in (`load` o `run`)
4. Fornire una rudimentale gestione degli errori

---

Per realizzare l'estensione che permette di implementare una cronologia dei comandi giá utilizzati ho
utilizzato la libreria `readline` per compilare il file é quindi necessario utilizzare uno dei seguenti
comandi:

- `gcc dsh.c -o dsh -lreadline`
- `make`
