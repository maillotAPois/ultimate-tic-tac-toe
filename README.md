# Ultimate Tic-Tac-Toe — IA C++

Projet C++ A3 Alternant ESILV (2025-2026). Implementation d'une IA pour
le jeu Ultimate Tic-Tac-Toe via la librairie `libUTTTLib.a` fournie.

**Niveau cible**: MEDIUM_2 (validation au minimum 80% de victoires en
mode Arene, egalites non comptees).

**Etat actuel**: ~55% de victoires (no-draw) sur MEDIUM_2, 100 parties.
En dessous du seuil de validation. Voir section *Resultats* pour le detail.

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
**elagage alpha-beta** et plusieurs techniques d'optimisation:

- **Iterative deepening** avec budget temps (200 ms/coup): on cherche
  a profondeur 1, 2, 3... tant qu'il reste du temps. Garantit toujours
  un coup valide meme si on coupe au milieu d'une iteration profonde.
- **Table de transposition** (TT) indexee par hash Zobrist. Persiste
  entre coups (cap 2M entrees). La PV move issue de la TT est testee
  en premier au coup suivant.
- **Killers moves**: 2 par ply, reinitialises entre coups.
- **Move ordering** racine et a depth >= 4 par eval statique.
- **Quiescence search** (qdepth max 6): a la frontiere d'horizon, on
  n'etend que les coups tactiques (gain de sous-grille ou gain global)
  pour eviter l'effet d'horizon.

### Heuristique d'evaluation

Du point de vue du joueur courant:
- `+/- WIN_VAL` aux positions terminales (avec attenuation de profondeur
  pour preferer les victoires rapides).
- Score par sous-grille via `evalSubBoard`: somme de `lineScore` sur
  les 8 lignes, ponderee par les poids positionnels (centre > coins >
  cotes). La sous-grille **forcee** (ou l'adversaire doit jouer ensuite)
  est multipliee par 3 -- ce qui s'y passe est imminent.
- Score meta: chaque ligne de sous-grilles capturees vaut 400 * lineScore.
- **Fork meta**: bonus/malus de +-2000 si >= 2 menaces meta simultanees.
- Bonus +30 quand on a le choix libre de la sous-grille de destination.

## Resultats

Mesures contre la reference du framework, 100 parties en mode Arene
(alternance X/O), budget 200 ms/coup, niveau MEDIUM_2:

- **Player win ratio: 51%**, IA: 41%, nuls: 8% (no-draw rate ~55%).

Le seuil de validation MEDIUM_2 est 80%. Les ameliorations qui ont
porte le score de ~45% (baseline minimax e675bd0) a 55% sont la
ponderation forced-sub * 3 et le fork meta. D'autres tentatives
(filtre block-immediate-loss, sub-fork dans evalSubBoard) ont degrade
ou ete dans la marge de variance.

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
