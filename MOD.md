# Cube World Mod (Branch `mod/main`)

Sams eigene Version: die Release (1.0.0-1) mit dem Klassen-Gameplay der Alpha (2013-07-20).

## Branches

| Branch | Inhalt | Checkout |
| --- | --- | --- |
| `main` | Stand von coremaze (`upstream`) | – |
| `linux-port` | Release unverändert, nur Plattform: Linux nativ, Windows (MSVC, MinGW) baut weiter | `~/Git/Infinite-Cubes` |
| `mod/main` | dieser Mod, baut auf `linux-port` auf | `~/Git/Infinite-Cubes-mod` |

Plattform-Korrekturen kommen nach `linux-port` und werden von dort nach `mod/main` gemergt (`git merge linux-port`); Gameplay
kommt nur nach `mod/main`. Der Mod-Code ist plattformneutral: Plattformcode bleibt in `release/platform/`.

## Aufbau

- `release/src/mod`, `release/include/mod`: Mod-Code (Namespace `mod`). Eingriffe in den Release-Code bleiben klein und
  rufen den Mod-Code auf.
- `release/data/mod/classes.json`: Klassen, Spezialisierungen, Skills und ihre Werte. Der Build kopiert sie neben das
  Programm; das Spiel liest sie bei Änderung neu (Live-Tuning).
- `tools/skill-editor`: Weboberfläche für die Skill-Daten (Python-Standardbibliothek, ohne Abhängigkeiten).
- Alpha-Grafiken (`gui.plx`, `data3.db` der Alpha) liegen nicht im Repo: `tools/import-alpha-assets.py` kopiert sie aus
  der eigenen Alpha-Kopie ins Laufzeitverzeichnis (`alpha/gui.plx`, `alpha/icons/*.png`).

## Ziele und Stand

1. [x] Branch, Builds (Linux, Windows MinGW), Laufzeitordner `~/Games/InfiniteCubes/mod-linux`, `launch.sh mod`
2. [x] Klassendaten `classes.json` mit Loader und Live-Reload
3. [x] Alpha-Fähigkeitsleiste: Maus links/rechts nach Waffe (die Release-Angriffe), Tasten 1–3 nach Klasse und
       Spezialisierung; Shift-Klassenskill, R-Ultimate und Ausweich-Skills der Release sind aus
4. [x] Fortschritt: Alpha-Level und Erfahrung aus Kämpfen (eigener Wert, das Release-Level zählt weiter die Artefakte),
       Skillpunkte (2 je Level, 1 je 4 Manawürfel), gespeichert hinter den Figurendaten
5. [x] Alpha-HUD: Lebens-, Erfahrungs-, Manawürfelbalken oben links; HP/MP und Fähigkeitsleiste mit Icons, Abklingzeit
6. [x] Alpha-Skillmenü (X): Spezialisierungen, Klassen-Skills (Kette), allgemeine Skills; Lernen, Zurückgeben und
       Spezialisierung wechseln beim Klassenlehrer gegen Gold
7. [~] Alpha-Tasten und Bewegung: X Skills, 1–3 Klassen-Skills, R reden/öffnen (E hebt auf und benutzt) sind da; die
       allgemeinen Skills Klettern, Schwimmen, Reiten, Gleiten und Segeln wirken wie Artefakte der Release (2 je Punkt,
       die Alpha-Kurve). Offen: Zähmen nach Skillstufe, G für Gleiter/Boot, Strg klettern, Shift zielen, Alpha-Bewegung
       ohne Ausweichrolle
8. [x] Weboberfläche: Skills Klassen zuordnen und duplizieren, Werte live ändern

## Benutzen

```sh
tools/import-alpha-assets.py ~/Downloads/cubeworld-20130624.zip ~/Games/InfiniteCubes/mod-linux   # einmal
~/Games/InfiniteCubes/launch.sh mod      # spielen
~/Games/InfiniteCubes/launch.sh editor   # Skill-Editor im Browser, wirkt sofort im laufenden Spiel
```

Für Tests: `CUBE_MOD_TEST_POINTS=n` gibt einer Figur ohne Punkte n Punkte in jedem Klassen-Skill,
`CUBE_MOD_TEST_LEVEL=n` hebt ihr Alpha-Level auf n.

## Alpha-Referenz

- Fähigkeitsleiste: `alpha/src/cube/GameController.cpp` (`m_barAbilities`, Aktivierung beim Bar-Input)
- Skill-Zuordnung und Boni: `alpha/src/cube/CreatureStats.cpp` (`abilitySkillIndex`, `skillBonus`, `levelBonus`)
- Skillmenü: `alpha/src/cube/widgets/SkillWidget.cpp`, Punktevergabe in `GameController.cpp`
- Ability-IDs: Alpha und Release zählen verschieden (Release meist +1 bis +32). `classes.json` nennt die Fähigkeiten beim
  Namen der Release (Tabelle in `release/src/mod/ClassData.cpp`), die Icons dazu stehen in `release/src/mod/AlphaHud.cpp`.
