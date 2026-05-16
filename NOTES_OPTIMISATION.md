# Notes d'optimisation IA — UTTT MEDIUM_2

Journal de travail pour amener l'IA au seuil de validation. **À tenir à jour.**

## Objectif

Valider **MEDIUM_2** en mode Arène : **≥ 80 % de victoires, égalités non comptées**,
soit `victoires / (victoires + défaites) ≥ 0,80`.

État : baseline HEAD `7d97415` ~52 %. Config consolidée actuelle **~59 %**.
Cible 80 % **non atteinte**.

## Métrique de référence

Seule la sortie du framework fait foi :

```
Game: Player win ratio : XX%      <- nous (RomThpt)
Game: IA win ratio : YY%          <- bot de reference
```

Nulles = `Winner O (IA AND PLAYER)`. `no-draw = Player / (Player + IA)`.
Le winner par partie : `Winner X (PLAYER)` = on gagne, `Winner X (IA)` = on perd.
Vérifié : l'étiquette `(PLAYER)`/`(IA)` du framework = qui a joué le symbole
gagnant. Pas d'inversion possible si on lit cette sortie directement.

## Contrainte de temps (CRITIQUE)

Le framework **plafonne le temps mesuré par coup à ~400 ms** (overhead IPC/wine
~135 ms compris). Symptome : lignes `Game: O - Timeout (NNN ms)`.

- 270 ms de budget : 0-1 timeout (limite effleurée).
- 800 ms : ~78 % des coups en timeout.
- 1500 ms : run corrompu (1260 timeouts, 66 parties pour 60).

=> Budget praticable **max ~250-270 ms**. Le levier temps est **épuisé**.
Config actuelle : 250 ms (marge de securite, ~7 timeouts/100 parties).

## BRUIT DE MESURE (critique — lire en premier)

Le binaire commité `a6770da`, **inchangé**, benché 3 fois : **62 %, 49 %, 48 %**.
=> Le bench 100 parties a une dispersion réelle de **~±15 pp**, pas ±5 pp.

Conséquence : **aucun effet < ~20 pp n'est mesurable** sur un run de 100
parties. La "progression 52→59→62 %" de la session est en grande partie du
bruit ; la vraie force du binaire commité est ~53 %, soit ≈ la baseline.

Prérequis avant toute optimisation sérieuse : **mesure fiable**
(5× 100 parties agrégées = 500 parties, ou régler le freeze des runs longs).

## Mesures (100 parties — NON FIABLES, voir bruit ci-dessus)

| Config | no-draw | Note |
|---|---|---|
| Baseline HEAD | ~52 % | 45/42/13 |
| Éval: dead-sub blocking | ~45-48 % | à plat (bruit) |
| Éval: refonte A+B+C (poids méta, contrôle) | ~51 % | à plat |
| Terminal `draw=0` | ~49 % | à plat |
| LMR + historique | ~48 % | +0,5 ply seulement |
| **TT tableau** | **~58 %** | commit `8984beb` |
| **Consolidé (TT + LMR + hist, 250 ms)** | **~59 %** | 55/38/7 |
| **Éval asymétrique pessimiste** | **~62 %** | commit `a6770da`, 58/35/7 |
| Asymétrie *adaptative* (selon avance) | ~53 % | régression — désactivait la pessimité à égalité de subs |
| Extensions tactiques (sub-win +1 ply) | ~43 % | régression — creuse les mauvaises lignes, baisse la profondeur nominale |

Progression : 52 % → 59 % (TT tableau) → 62 % (éval asymétrique).
L'éval pessimiste a surtout débloqué le jeu en O : **71 %** (vs 33 % au
départ). En X : 59 %. Cible 80 % toujours non atteinte.

### Profilage (important)

`evaluate()` = **36 %** du temps de recherche (pas 80 %). Une éval
incrémentale ne donnerait qu'un speedup ×1,56 ≈ +0,5 ply — **insuffisant**
pour atteindre depth 11. Refactor non rentable, écarté.

Profondeur de recherche : **depth 8-9** en milieu de partie (270 ms),
~10,7 à 1500 ms (mais 1500 ms = timeouts, inutilisable).

## Diagnostic des défaites

Trace des 16 défaites d'un run : **toutes** suivent le même scénario.
On atteint une position que l'éval juge gagnante (+800 à +1700), puis le
score **se dégrade lentement sur 8-10 coups** jusqu'à -99000. Jamais une
bourde tactique : une hémorragie.

Cause : l'éval **surévalue** ; le piège adverse est à ~11-13 plies et la
recherche milieu de partie (depth 8-9) **ne le voit pas venir**. Quand on
s'en approche, depth 11-12 révèle la vérité, trop tard.

Asymétrie : **68 % en X, 33 % en O**. Notre moteur est de **force ≈ MEDIUM_2** ;
le score est plafonné par l'avantage du premier coup.

## Ce qui est tranché

- **L'éval n'est PAS le levier** : 4+ refontes, toutes à plat. Sa structure
  est correcte ; la surévaluation vient de l'horizon trop court, pas des poids.
- **Le temps n'est PAS un levier** : framework plafonné à ~400 ms.
- **La recherche est correcte** : apply/undo vérifié (hash Zobrist self-inverse,
  Board sans cache), négamax/PVS/TT sains. Pas de bug de correctness trouvé.
- **TT tableau** : vrai gain. `unordered_map` -> tableau direct-mapped 4M
  entrées, remplacement préférant la profondeur, clé vérifiée.
- **LMR** : rend peu en UTTT (branching ~6-9 en sous-grille forcée) : +0,5 ply.

## MESURE FIABLE : le harnais self-play

`selfplay.cpp` (cible `make selfplay`, compilé **natif** clang++) fait jouer
deux MinimaxPlayer l'un contre l'autre, 162 parties (81 ouvertures × 2
couleurs), **sans framework ni wine**. Déterministe => **bruit nul**.
C'est l'outil de mesure A/B fiable qui manquait toute la session.

Usage : `./selfplay <depthA> <depthB>` (profondeur fixe, budget illimité).

## PREUVE : la profondeur est LE levier (self-play, bruit nul)

| Match | Résultat A |
|---|---|
| depth 9 vs depth 9 | 50,4 % (sanité OK) |
| **depth 11 vs depth 9** | **75,6 %** |
| **depth 12 vs depth 9** | **86,7 %** |
| depth 11 + beam(12) vs depth 9 | 73,4 % |

Comme depth-9 ≈ MEDIUM_2 : **depth-11 contre le bot ≈ 75 %, depth-12
≈ 85 %+**. La cible exacte pour valider 80 % est **depth-12**. La
profondeur règle le problème — prouvé, chiffré, sans bruit.

## Le mur : la vitesse

depth-11 coûte **~1,15 s/coup en natif**, soit **~3,5 s/coup sous wine**
(wine ~2,4× plus lent, + iterative deepening). Budget praticable : **250 ms**.

| Cible | Force ≈ | Speedup requis |
|---|---|---|
| depth 10 | ~63 % | ~5× |
| depth 11 | ~75 % | ~14× |
| depth 12 | ~80 %+ | ~30-40× |

**Atteindre 80 % = depth ~12 = moteur ~30× plus rapide.**

Pistes testées pour la vitesse, **insuffisantes** :
- buffers de scores pré-alloués : aucun gain mesurable.
- beam pruning (limiter à 12 coups/nœud) : garde la force (73 %) mais ne
  gagne presque rien en vitesse — la plupart des nœuds sont en sous-grille
  forcée (≤9 coups), déjà étroits ; le beam ne mord que sur les nœuds en
  libre choix, minoritaires.
- profilage : `evaluate()` = 36 % du temps => éval incrémentale ne donne
  que ~1,56× (depth +0,5 ply). Loin du compte.

## Conclusion : ce qu'il faudrait pour 80 %

Une **réécriture moteur haute performance** (~30× la vitesse actuelle) :
bitboards intégraux sur tout le plateau 9×9, zéro `std::vector` et zéro
copie de `GameState` dans le chemin chaud, génération de coups en masques
de bits, éval incrémentale, négamax sans abstraction. C'est un **projet
de plusieurs jours**, niveau moteur de jeu d'échecs.

Le minimax actuel (abstractions GameState/Board, vectors, éval recalculée)
plafonne ~depth 9 dans le budget => ~égalité avec MEDIUM_2.

## Pièges méthodologiques (NE PAS REFAIRE)

1. **Bruit du bench** : 100 parties = ±5 pp. Un effet < 10 pp est indétectable.
2. **`uttt.exe` se fige à la fin** de chaque run : `pkill -x uttt.exe`.
3. **Runs longs instables** : éviter > 100 parties d'un coup.
4. **Compter les défaites via `state.winner()` local est FAUX** : la boucle
   `while(!game.isFinish())` sort avant le `getMove()` du coup gagnant
   adverse. Utiliser `game.getWinner()` ou la sortie framework.

## Architecture (rappel)

```
main.cpp              Boucle de jeu, 100 parties MEDIUM_2 Arène
src/MinimaxPlayer     negamax + alpha-beta + PVS + ID + TT tableau
                      + killers + historique + LMR + quiescence
src/GameState         État + règles UTTT, hash Zobrist incrémental
src/UltimateBoard     Méta-grille 3x3 de Board
src/Board             Sous-grille 3x3 (bitboards)
```

## Commandes utiles

```sh
make all
WINEDEBUG=-all wine ./uttt.exe 2>/dev/null         # bench (stdout = resultats)
pkill -x uttt.exe                                  # tuer le process fige
```
