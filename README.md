# AOS — Auguste OS

AOS est un firmware personnalisé pour la NumWorks N0110. Son premier écran est un accueil sombre avec une grille de trois colonnes, un indicateur de batterie et un catalogue vide. Les six cases dessinées à l'écran sont des emplacements visuels. Le système démarre sur cet accueil et contient zéro application.

## Images pour les slots de démarrage

Le [pack de la première version](firmware/AOS-0.1.0-N0110.zip) contient les images des deux slots et leurs sommes SHA-256. Les images ont le format de démarrage d'Upsilon pour N0110. Phi est le chargeur de démarrage visé. Le démarrage et la reprise de veille avec la version de Phi installée sur la calculatrice restent à valider sur l'appareil.

| Image | Adresse de début | Format |
| --- | --- | --- |
| `AOS-N0110-slot-A.dfu` | `0x90000000` | DfuSe avec adresse intégrée |
| `AOS-N0110-slot-B.dfu` | `0x90400000` | DfuSe avec adresse intégrée |
| `AOS-N0110-slot-A.bin` | `0x90000000` | Image brute liée pour A |
| `AOS-N0110-slot-B.bin` | `0x90400000` | Image brute liée pour B |

Identifie d'abord le slot libre et la version de Phi. Le flash efface le contenu du slot choisi. Chaque image couvre uniquement de la flash externe dans son slot et le build vérifie cette limite. La compilation et les tests se déroulent sans connexion à une calculatrice.

## Compiler

Sous Debian, Ubuntu ou WSL, installe les dépendances, puis lance le build depuis le dépôt.

```sh
sudo apt-get install build-essential git python3 pkg-config libfreetype-dev gcc-arm-none-eabi binutils-arm-none-eabi
bash tools/build.sh
```

Le script récupère une révision précise d'Upsilon dans `.build/`, utilise ses pilotes Ion/Kandinsky et dépose les quatre images avec leurs sommes SHA-256 dans `dist/`. Le workflow GitHub Actions fournit aussi ces fichiers dans l'artefact `AOS-N0110`.

## Ajouter une application native plus tard

- `src/apps.h` contient le catalogue compilé. Chaque entrée déclare son nom, son libellé court et ses fonctions.
- `src/launcher.cpp` gère la navigation, l'activation, l'entrée et la sortie d'une application. Les flèches parcourent la grille, OK ou EXE ouvre l'élément choisi et Retour ou Maison ramène à l'accueil.
- `src/main.cpp` gère l'affichage, la batterie, les événements de clavier, la veille et le réveil. Marche/Arrêt déclenche la mise en veille, ainsi que cinq minutes sans touche.

Une application native enregistre une fonction `draw(KDContext &)`. L'origine de sa zone de dessin est le coin supérieur gauche de l'espace sous l'en-tête, qui mesure 320 × 191 pixels. Elle peut ajouter les fonctions facultatives `enter()`, `leave()` et `handle(Input)`. Le gestionnaire reçoit la touche normalisée et l'identifiant brut d'Ion. Il renvoie `true` si l'écran doit être redessiné. Pour conserver une application dans l'image, ajoute ses fichiers au répertoire `src/`, ses sources C++ au build et son entrée au catalogue. Les applications partagent l'espace mémoire du firmware.

Initialise l'état au chargement de l'application dans `enter()`, ou utilise une initialisation constante. Le code de démarrage rejette les constructeurs globaux exécutés au démarrage. Le build vérifie cette contrainte avant de publier la paire d'images.

## Crédits

AOS utilise Ion, Kandinsky et le format de démarrage d'[Upsilon](https://github.com/UpsilonNumworks/Upsilon), issu d'Omega et d'Epsilon. Le script spécialise la liste des sources compilées et adapte les valeurs sentinelles des champs d'applications externes. Voir la [licence CC BY-NC-SA 4.0](LICENSE.md).
