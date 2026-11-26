# AOS — Auguste OS

AOS est un firmware personnalisé pour la NumWorks N0110. Son premier écran est un accueil sombre avec une grille de trois colonnes, un indicateur de batterie et la grille native d’Upsilon avec l’icône de Calculation. Depuis l’accueil, sélectionne l’icône avec les flèches et appuie sur OK ou EXE. La touche Maison ramène à l’accueil.

## Télécharger AOS pour N0110

Les liens « Dernière release » suivent automatiquement la version stable désignée par GitHub. Avant la première publication dans Releases, utilise les liens directs v0.3.2.

| Slot | Format | Adresse | Dernière release | v0.3.2 |
| --- | --- | --- | --- | --- |
| A | `.dfu` | `0x90000000` | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases/latest/download/AOS-N0110-slot-A.dfu) | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/raw/refs/heads/main/firmware/v0.3.2/AOS-v0.3.2-N0110-slot-A.dfu) |
| A | `.bin` | `0x90000000` | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases/latest/download/AOS-N0110-slot-A.bin) | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/raw/refs/heads/main/firmware/v0.3.2/AOS-v0.3.2-N0110-slot-A.bin) |
| B | `.dfu` | `0x90400000` | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases/latest/download/AOS-N0110-slot-B.dfu) | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/raw/refs/heads/main/firmware/v0.3.2/AOS-v0.3.2-N0110-slot-B.dfu) |
| B | `.bin` | `0x90400000` | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases/latest/download/AOS-N0110-slot-B.bin) | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/raw/refs/heads/main/firmware/v0.3.2/AOS-v0.3.2-N0110-slot-B.bin) |

Sommes SHA-256 de la [dernière release](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases/latest/download/SHA256SUMS) et de la [v0.3.2](https://github.com/augustecolantonio6-netizen/OS_Numworks/raw/refs/heads/main/firmware/v0.3.2/SHA256SUMS).

Les images ont le format de démarrage d'Upsilon pour N0110. Phi est le chargeur de démarrage visé. Le démarrage et la reprise de veille avec la version de Phi installée sur la calculatrice restent à valider sur l'appareil.

Identifie d'abord le slot libre et la version de Phi. Le flash efface le contenu du slot choisi. Chaque image couvre uniquement de la flash externe dans son slot et le build vérifie cette limite. La compilation et les tests se déroulent sans connexion à une calculatrice.

## Vérifier la version démarrée

Une fois le transfert terminé, débranche le câble USB et sélectionne le slot que tu viens de flasher dans le menu de démarrage. Le menu varie selon la version du chargeur. L’accueil mis à jour affiche **v0.3.2** en haut et l’icône **Calculation** sélectionnée. Si l’accueil affiche encore « Aucune application », vérifie le fichier téléchargé et le slot sélectionné avant de reflasher.

## Écran de connexion USB

Branche un câble USB alimenté pour afficher l'écran « THE CALCULATOR IS CONNECTED ». Appuie sur Retour ou débranche le câble pour revenir à l'écran précédent. Après Retour, débranche et rebranche le câble pour réafficher l'écran.

AOS réutilise le [contrôleur d'écran USB d'Upsilon](https://github.com/UpsilonNumworks/Upsilon/blob/539b90a9a8aaf3db9451da13505f938116650f14/apps/usb/usb_connected_controller.cpp), son thème clair et son indicateur de batterie. Cet écran indique la présence du câble. Le texte du milieu décrit cette connexion.

## Compiler

Sous Debian, Ubuntu ou WSL, installe les dépendances, puis lance le build depuis le dépôt.

```sh
sudo apt-get install build-essential git python3 pkg-config libfreetype-dev libpng-dev gcc-arm-none-eabi binutils-arm-none-eabi
bash tools/build.sh
```

Le script récupère une révision précise d'Upsilon dans `.build/`, utilise ses pilotes Ion/Kandinsky et dépose les quatre images avec leurs sommes SHA-256 dans `dist/`. Le workflow GitHub Actions fournit aussi ces fichiers dans l'artefact `AOS-N0110`.

## Publier une version

Pousse un tag de version commençant par `v` depuis le commit à publier.

```sh
git tag v0.3.2
git push origin v0.3.2
```

GitHub Actions compile les deux slots depuis le tag, lance les tests et publie les quatre images, le fichier `SHA256SUMS` et la licence dans [Releases](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases) si le build réussit.

## Intégration de Calculation

Le build sélectionne l’application native `calculation` d’Upsilon avec `EPSILON_APPS=calculation`. Il compile les sources d’origine avec Escher et le moteur mathématique Poincaré. L’accueil réutilise le contrôleur de grille, les cellules d’applications et le conteneur d’Upsilon. Le nom, l’icône et l’ouverture de l’app proviennent du même registre natif. `tools/upsilon-aos.patch` adapte le démarrage, le thème de l’accueil et la gestion de l’écran USB.

## Crédits

AOS utilise Ion, Kandinsky, Escher, Poincaré et le format de démarrage d'[Upsilon](https://github.com/UpsilonNumworks/Upsilon), issu d'Omega et d'Epsilon. Le script spécialise la liste des sources compilées et adapte les valeurs sentinelles des champs d'applications externes. Voir la [licence CC BY-NC-SA 4.0](LICENSE.md).
