Vorrei applicassi in modo strutturato le seguenti modifiche:
- crea un file .json nella root del progetto dove definisci i metadati del progetto quali: nome della libreria, versione della libreria (che deve essere sincronizzata con il file VERSION nella root) , dipendenze (nome libreria + versione libreria + path [solo librerie scritte da me che vengono salvate in D:\GeekyTechRepos\_shared_lib]) e altri dati che possono essere utili, depoy folder. Questo file deve essere letto da uno script python che devi inserire dentro scripts. Leggi dopo per capire cosa deve fare.
- Poiche la libreria deve essere compilabile sotto windows, linux, docker, e deve essere utilizzata in framework tipo Qt che non supportano clang (C:\eng_apps\Qt\6.10.1 : vedo mingw), vorrei modificassi il CMakePresets.json lasciando solamente le toolchain che non creano problemi di compatibilità API/ABI, quindi lascia solamente mingw, distinguendo sempre debug e release, windows, linux (vorrei provare a compilare sotto wsl), docker.
- Poiche devo compilare sotto windows e linux, rimuovi tutti gli script .bat in /scripts e sostituisci tutto con uno script python che eseguito mostra comandi interattivi su console, in modo colorato. 
- Crea un virtual environment , un file requirements.txt con le librerie necessarie per fare quanto detto nel punto sopra, se servono librerie aggiuntive, per console con colori. 
- Lo script python (project_manager.py) deve poter essere eseguito dopo attivazione virtual environment (chiamalo pyvenv), e tramite questo script l'utente deve poter fare le seguenti azioni: 1. scegliere il preset: windows-mingw32-debug/release windows-mingw64-debug/release, docker-mingw-debug/release, linux-mingw-debug/release 2. fare la configurazione cmake 3. fare la build 4. fare clear all della folder build 5. eseguire gli unit test e visualizzare i risultati su file html su browser 6. fare deploy della libreria in locale o su shared_folder, definita nel file metadata  (folder x deploy) 7. uscire
- Garantisci distribuzione della libreria (.dll o .so) senza dove distribuire i runtime (tipo libstdc++.dll o simili) 
- Se la libreria che vogliamo compilare dipende da librerie scritte da me condivise nella folder D:\GeekyTechRepos\_shared_lib, devi verificare che in quella folder esista la struttura seguente:
D:\GeekyTechRepos\_shared_lib\<nome_libreria>\<version>\<preset>:
esempio: D:\GeekyTechRepos\_shared_lib\lib-test\1.0.0\windows-mingw64-debug\bin
D:\GeekyTechRepos\_shared_lib\lib-test\1.0.0\windows-mingw64-debug\include
D:\GeekyTechRepos\_shared_lib\lib-test\1.0.0\windows-mingw64-debug\lib
Quindi il CMakeLists.txt deve essere in grado di considerare i files .h in quella folder,
e linkare staticamente la libreria dinamica in bin e lib, in modo da non doverla distribuire a corredo della libreria che stiamo compilando.
- Aggiorna il README.md in modo dettagliato, definendo i prerequisiti (python installato su pc x poter creare venv), crea sezione che spiega come creare venv e tutto quello definito sopra.


rimuovi tutti i files relativi a clang, esempio clang-format, la folder triplets a cosa serve?

quando faccio deploy per docker, voglio vedere nella folder shared_lib la stessa struttura: bin , include , lib
inoltre in include non voglio che crei ulteriore sottocartella, ma metti i files .h/.hpp direttamente in include. Risolvi

assicurati che tutte le fmu in data supportino sia windows che linux, quindi devono avere la struttura di cartelle corretta come definito dallo standard.

rimuovi tutte dipendenze vcpkg