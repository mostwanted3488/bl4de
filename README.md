# bl4de

Visualizer audio (VST3 / AU, macOS Apple Silicon + Intel) par **@buyorbealoser**.
Un stickman à l'écharpe rouge tranche des vagues d'ennemis à l'infini, en rythme avec le morceau.
Le son traverse le plugin sans être modifié.

## Installation (Mac)

1. Télécharge `bl4de-mac.zip` : https://github.com/mostwanted3488/bl4de/releases/download/latest/bl4de-mac.zip — puis dézippe-le.
2. Copie les fichiers :
   - `bl4de.vst3` → `~/Library/Audio/Plug-Ins/VST3/`
   - `bl4de.component` → `~/Library/Audio/Plug-Ins/Components/`
3. Le plugin n'est pas signé par Apple : dans le Terminal, lance une fois
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/bl4de.vst3 ~/Library/Audio/Plug-Ins/Components/bl4de.component
   ```
4. Dans FL Studio : *Options → Manage plugins → Find installed plugins*, puis ajoute **bl4de** en effet sur le Master (ou sur la piste de batterie).

## Utilisation

- Le combat se cale tout seul sur le tempo et la position du projet.
- Kicks, snares et hi-hats du son qui passe dans le plugin déclenchent les coups.
- ⚔ (en bas à gauche) : armurerie — katana, faux, pistolet, mix aléatoire.
- ▭ : mode bandeau (fenêtre 4:1), ▢ pour revenir.
- Fenêtre redimensionnable (800 × 450 par défaut).

## Build

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target bl4de_VST3 bl4de_AU
```
JUCE 8 est téléchargé automatiquement par CMake.
