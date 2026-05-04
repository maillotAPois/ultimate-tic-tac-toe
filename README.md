# Ultimate Tic-Tac-Toe — IA C++

Projet C++ A3 Alternant ESILV (2025-2026). Implementation d'une IA pour
le jeu Ultimate Tic-Tac-Toe via la librairie `libUTTTLib.a` fournie.

## Architecture

```
main.cpp          - Boucle de jeu, instancie GameState + AIPlayer
src/
  Board.h/cpp      - Sous-grille 3x3, detection de gagnant
  UltimateBoard.h/cpp - Meta-grille 3x3 de Board, vue meta
  Move.h           - Coup (row, col) en coordonnees globales
  GameState.h/cpp  - Etat complet d'une partie + regles UTTT
  AIPlayer.h       - Interface (chooseMove)
  RandomPlayer.h/cpp  - Baseline pour tests de non-regression
  MinimaxPlayer.h/cpp - IA principale (negamax + alpha-beta + ID)
  Evaluator.h/cpp  - Fonction d'evaluation heuristique
```

## Algorithme

L'IA s'appuie sur:
- **Negamax** (variante symetrique du minimax) avec **elagage alpha-beta**.
- **Iterative deepening**: recherche a 1, 2, 3, ... plis tant que le
  budget temps (150 ms par defaut) le permet. Si l'on est interrompu
  en plein milieu d'une profondeur, on conserve le meilleur coup
  trouve a la profondeur precedente.
- **Move ordering**: les scores obtenus a la profondeur N servent
  a trier les coups pour la profondeur N+1, ce qui ameliore drastiquement
  les coupures alpha-beta.

### Heuristique d'evaluation

Du point de vue du joueur courant, somme ponderee de:
1. Sous-grilles gagnees, ponderees par leur position dans la meta-grille
   (centre = 4, coins = 3, bords = 2).
2. Menaces (2-en-ligne) sur la meta-grille — bonus exponentiel pour les
   "forks" (2 menaces simultanees).
3. Menaces (2-en-ligne) dans les sous-grilles non terminees.
4. Possession du centre du plateau global (4, 4).
5. Liberte de choix de la sous-grille suivante.

## Compilation

Projet CodeBlocks fourni: ouvrir `UTTT_Template.cbp`. La compilation
necessite GCC, `-std=c++17` est active. Voir le sujet pour les DLLs
Allegro requises.

## Choix de programmation pour la soutenance

- **POO** stricte: aucune fonction libre metier; chaque responsabilite
  est portee par une classe (Board, UltimateBoard, GameState, AIPlayer,
  Evaluator, MinimaxPlayer).
- **STL**: `std::array` (taille fixe, pas d'allocation dynamique pour
  les grilles), `std::vector` pour les listes de coups, `std::sort`
  + lambdas pour le move ordering, `std::chrono::steady_clock` pour
  le budget temps.
- **Symetrie negamax** plutot que minimax explicite: divise la taille
  du code de la recherche par 2 et evite les bugs MIN/MAX.
- **Sentinelle d'exception** (`TimeOut`) pour interrompre proprement
  une recherche depassant le budget temps, sans polluer le code de
  retours d'erreur.
