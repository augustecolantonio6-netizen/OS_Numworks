# AOS — Auguste OS

AOS est un firmware personnalisé pour la NumWorks N0110. Son premier écran est un accueil sombre avec une grille de trois colonnes, un indicateur de batterie et la grille native d’Upsilon avec les icônes de Calculation et LaTeX. Depuis l’accueil, sélectionne l’icône avec les flèches et appuie sur OK ou EXE. La touche Maison ramène à l’accueil.

## Télécharger AOS pour N0110

Les liens « Dernière release » suivent automatiquement la version stable désignée par GitHub. Avant la première publication dans Releases, utilise les liens directs v0.5.0.

| Slot | Format | Adresse | Dernière release | v0.5.0 |
| --- | --- | --- | --- | --- |
| A | `.dfu` | `0x90000000` | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases/latest/download/AOS-N0110-slot-A.dfu) | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/raw/refs/heads/main/firmware/v0.5.0/AOS-v0.5.0-N0110-slot-A.dfu) |
| A | `.bin` | `0x90000000` | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases/latest/download/AOS-N0110-slot-A.bin) | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/raw/refs/heads/main/firmware/v0.5.0/AOS-v0.5.0-N0110-slot-A.bin) |
| B | `.dfu` | `0x90400000` | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases/latest/download/AOS-N0110-slot-B.dfu) | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/raw/refs/heads/main/firmware/v0.5.0/AOS-v0.5.0-N0110-slot-B.dfu) |
| B | `.bin` | `0x90400000` | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases/latest/download/AOS-N0110-slot-B.bin) | [Télécharger](https://github.com/augustecolantonio6-netizen/OS_Numworks/raw/refs/heads/main/firmware/v0.5.0/AOS-v0.5.0-N0110-slot-B.bin) |

Sommes SHA-256 de la [dernière release](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases/latest/download/SHA256SUMS) et de la [v0.5.0](https://github.com/augustecolantonio6-netizen/OS_Numworks/raw/refs/heads/main/firmware/v0.5.0/SHA256SUMS).

Les images ont le format de démarrage d'Upsilon pour N0110. Phi est le chargeur de démarrage visé. Le démarrage et la reprise de veille avec la version de Phi installée sur la calculatrice restent à valider sur l'appareil.

Identifie d'abord le slot libre et la version de Phi. Le flash efface le contenu du slot choisi. Chaque image couvre uniquement de la flash externe dans son slot et le build vérifie cette limite. La compilation et les tests se déroulent sans connexion à une calculatrice.

## Vérifier la version démarrée

Une fois le transfert terminé, débranche le câble USB et sélectionne le slot que tu viens de flasher dans le menu de démarrage. Le menu varie selon la version du chargeur. L’accueil mis à jour affiche **v0.5.0** en haut et les icônes **Calculation** et **LaTeX**. Si l’accueil affiche encore « Aucune application », vérifie le fichier téléchargé et le slot sélectionné avant de reflasher.

## Écran de connexion USB

Branche un câble USB alimenté pour afficher l'écran « THE CALCULATOR IS CONNECTED ». Appuie sur Retour ou débranche le câble pour revenir à l'écran précédent. Après Retour, débranche et rebranche le câble pour réafficher l’écran. OK reste sans effet sur cet écran. À sa fermeture, les frappes sont filtrées pendant 180 ms.

AOS réutilise le [contrôleur d'écran USB d'Upsilon](https://github.com/UpsilonNumworks/Upsilon/blob/539b90a9a8aaf3db9451da13505f938116650f14/apps/usb/usb_connected_controller.cpp), son thème clair et son indicateur de batterie. AOS ouvre le service USB/DFU d’Upsilon quand l’ordinateur détecte la calculatrice. Avec un câble de données, ouvre ton outil de connexion NumWorks ou DFU alors qu’AOS tourne sur le slot A. La fermeture d’une session qui ne fait que transférer des données revient à AOS. Une mise à jour du firmware peut redémarrer la calculatrice. Le mode examen conserve ses restrictions USB.

## Éditeur LaTeX

- **OK** sur un nom ouvre le script. **OK** sur **Nouveau script** crée un document. La flèche **Droite** sur un script ouvre ses options et permet de le renommer. Les noms utilisent des minuscules, des chiffres et des tirets bas.
- Dans l’éditeur, le numéro à gauche indique la ligne. Les touches de racine, de fraction et de puissance saisissent des formules en 2D. **EXE** enregistre la ligne et avance. **Shift + Haut/Bas** enregistre la ligne et se déplace dans le script. **Retour** enregistre et revient à la liste.
- Chaque script possède son propre fichier `.py` pour le transfert USB. Dans [My NumWorks](https://my.numworks.com/), récupère le fichier et copie son contenu brut dans un document `.tex`. L’app LaTeX ne propose aucune commande d’exécution. Le fichier utilise des blocs `\[ ... \]` de LaTeX pur.

L’app prend en charge une formule par ligne. Elle conserve les sources éditables et le LaTeX exporté dans deux enregistrements séparés. Les anciens fichiers `aos_latex.py` sont conservés comme exports sans source éditable. Exporte tes scripts avant un RESET ou un nouveau flash. La récupération via le site My NumWorks reste à vérifier sur la N0110.

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
git tag v0.5.0
git push origin v0.5.0
```

GitHub Actions compile les deux slots depuis le tag, lance les tests et publie les quatre images, le fichier `SHA256SUMS` et la licence dans [Releases](https://github.com/augustecolantonio6-netizen/OS_Numworks/releases) si le build réussit.

## Intégration de Calculation

Le build sélectionne l’application native `calculation` d’Upsilon avec `EPSILON_APPS=calculation`. Il compile les sources d’origine avec Escher et le moteur mathématique Poincaré. L’accueil réutilise le contrôleur de grille, les cellules d’applications et le conteneur d’Upsilon. Le nom, l’icône et l’ouverture de l’app proviennent du même registre natif. `tools/upsilon-aos.patch` adapte le démarrage, le thème de l’accueil et la gestion de l’écran USB.

## Crédits

AOS utilise Ion, Kandinsky, Escher, Poincaré et le format de démarrage d'[Upsilon](https://github.com/UpsilonNumworks/Upsilon), issu d'Omega et d'Epsilon. Le script spécialise la liste des sources compilées et adapte les valeurs sentinelles des champs d'applications externes. Voir la [licence CC BY-NC-SA 4.0](LICENSE.md).
