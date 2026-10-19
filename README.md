# MemoMind Space Invader

Jeu de type Space Invaders pour les lunettes MemoMind, piloté par les mouvements
de tête et réglable depuis le téléphone.

Application **pairée** :

- **lunettes** : plugin natif GM (`com.dynag.invaders.glass`) — rendu LVGL, IMU,
  protocole Bluetooth `gm.invaders` ;
- **téléphone** : plugin Web (`com.dynag.invaders.phone`) — sensibilité, cadence
  de tir, mode de tir, bouton Nouvelle partie, score/vague/vies en direct.

## Commandes

Lunettes (boutons) :

- **tête gauche / droite** : déplacer le vaisseau (axe horizontal `gyro_x - gyro_y`) ;
- **bouton** : en mode tir automatique, met en pause / reprend ; en mode bouton,
  tire ;
- **appui long 2 s** : quitter (arc de progression autour de l'écran) ;
- **incliner la tête vers le haut 30° pendant 3 s** : quitter aussi.

Téléphone :

- **Sensibilité** (1–10) : amplitude du déplacement pour un même mouvement de tête ;
- **Mode de tir** : automatique ou bouton ;
- **Cadence** (1–10) : intervalle entre deux tirs automatiques ;
- **Nouvelle partie** : relance une partie.

## Protocole `gm.invaders` 1.0

| Canal | Valeur | Sens |
| --- | --- | --- |
| config | `0x4943` ('IC') | téléphone → lunettes |
| event | `0x4945` ('IE') | lunettes → téléphone |

Téléphone → lunettes (5 octets) : `[version, commande, sensibilité, mode, cadence]`

- commande `1` = configuration (sensibilité 1–10, mode 0 = auto / 1 = bouton,
  cadence 1–10) ;
- commande `2` = nouvelle partie.

Lunettes → téléphone (8 octets, toutes les 400 ms) :
`[version, event=1, score lo, score hi, vague, vies, en cours, état]`

## Structure

```
glasses/               sources C du plugin lunettes (GlassSDK/examples/invaders)
phone/                 sources du plugin Web (PhoneSDK/examples/invaders)
glasses-homeassistant/ sources Home Assistant (lunettes)
phone-homeassistant/   sources Home Assistant (téléphone)
builds/                paquets prêts à installer (invaders + homeassistant)
```

## Installation

1. `builds/invaders.gmp` — plugin lunettes, à importer dans Studio avec
   `glasses/manifest.json` à côté ;
2. `builds/invaders-1.0.0.mmpkg` — application téléphone ;
3. `builds/invaders-pair-1.0.0.zip` — paire exportée (GMP + MMPKG + revue).

## Langues

Français (défaut), anglais, espagnol, italien, allemand, chinois simplifié.
La langue suit la locale du téléphone pour l'interface Web ; les lunettes
utilisent la locale de l'hôte.
