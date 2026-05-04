# Ultimate Tic-Tac-Toe — IA C++

Projet C++ A3 Alternant ESILV (2025-2026). Implementation d'une IA pour
le jeu Ultimate Tic-Tac-Toe via la librairie `libUTTTLib.a` fournie.

**Niveau cible**: MEDIUM_2 (validation au minimum 80% de victoires en
mode Arene, egalites non comptees).

## Architecture

```
main.cpp          - Boucle de jeu, instancie GameState + AIPlayer
src/
  Board.h/cpp      - Sous-grille 3x3, detection de gagnant
  UltimateBoard.h/cpp - Meta-grille 3x3 de Board, vue meta
  Move.h           - Coup (row, col) en coordonnees globales
  GameState.h/cpp  - Etat complet d'une partie + regles UTTT
  AIPlayer.h       - Interface (chooseMove)
  MinimaxPlayer.h/cpp - IA: negamax + alpha-beta + eval simple
```

## Algorithme

L'IA s'appuie sur un **negamax** (variante symetrique du minimax) avec
**elagage alpha-beta** a profondeur fixe (4 demi-coups par defaut).

### Heuristique d'evaluation

Du point de vue du joueur courant:
- `+VALEUR_GAIN` si le joueur courant a gagne la partie
- `-VALEUR_GAIN` s'il a perdu
- Sinon: `SUB_VAL * (sous-grilles gagnees - sous-grilles perdues)`

Cette heuristique simple suffit pour valider les niveaux EASY et MEDIUM.

## Compilation

Projet CodeBlocks fourni: ouvrir `UTTT_Template.cbp`. La compilation
necessite GCC, `-std=c++17` est active. Voir le sujet pour les DLLs
Allegro requises.

## Choix de programmation pour la soutenance

- **POO** stricte: aucune fonction libre metier; chaque responsabilite
  est portee par une classe (Board, UltimateBoard, GameState, AIPlayer,
  MinimaxPlayer).
- **STL**: `std::array` (taille fixe, pas d'allocation dynamique pour
  les grilles), `std::vector` pour les listes de coups.
- **Symetrie negamax** plutot que minimax explicite: divise la taille
  du code de la recherche par 2 et evite les bugs MIN/MAX.
- **Alpha-beta**: coupure beta des qu'on a prouve qu'une branche est
  pire que ce qu'on a deja trouve. Reduit massivement le nombre de
  noeuds explores.
