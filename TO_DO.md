0. [FATTO] version.hpp ora viene generato e installato flat in include/version.hpp
   (non piu' in include/libfmu_toolkit/). Cambiato l'#include in
   lib_fmu_toolkit.hpp da <libfmu_toolkit/version.hpp> a "version.hpp",
   src/CMakeLists.txt genera il file flat, e CMakeLists.txt ha un solo
   install(FILES ...) invece di due. Redeploy fatto per windows-mingw64-debug
   e windows-mingw64-release, cartelle include/libfmu_toolkit/ residue rimosse.
1. [VERIFICATO, nessuna azione] controllato con objdump -p su
   libfmutoolkit.dll: l'unica dipendenza runtime non di sistema e'
   libwinpthread-1.dll - libxml2/libzip sono davvero linkate staticamente
   (FMUTOOLKIT_MINGW_STATIC_RUNTIME), quindi non serve copiare
   libxml2.dll/libzip.dll in bin/. I messaggi "Checking for module
   'libxml-2.0'/'libzip'" sono solo la risoluzione a configure-time via
   pkg-config, richiesta anche per il linking statico.
1. dentro /docs crea in user_manual il file USER_MANUAL.md, verboso e dettagliato in modo da essere distribuito con la dll.
2. crea il file di spec srs.md a partire dal template che trovi dentro la folder /docs/spec/srs.md: devi solo aggiungere i dati mantenendo la struttura del file
3. popola i files adr/adr.md e architecture/architecture.md
4. fai commit atomici, per categoria, di tutti i files modificat, in inglese, senza usa coauthored by claude nel commit msg