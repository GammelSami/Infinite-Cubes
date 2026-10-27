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
  der eigenen Alpha-Kopie ins Laufzeitverzeichnis (`alpha-gui.plx`, `alpha-icons.db`).

## Ziele und Stand

1. [ ] Branch, Builds (Linux, Windows MinGW), Laufzeitordner `~/Games/InfiniteCubes/mod-linux`, `launch.sh mod`
2. [ ] Klassendaten `classes.json` mit Loader und Live-Reload
3. [ ] Alpha-Fähigkeitsleiste: Maus links/rechts nach Waffe, Tasten 1–3 nach Klasse und Spezialisierung
4. [ ] Fortschritt: Erfahrung aus Kämpfen, Level, Skillpunkte (2 je Level, 1 je 4 Manawürfel), gespeichert pro Figur
5. [ ] Alpha-HUD: Lebens-, Mana-, Erfahrungsbalken, Fähigkeitsleiste mit Icons und Abklingzeit
6. [ ] Alpha-Skillmenü (X): Klassen-Skills (Kette: 5 Punkte im vorigen) und allgemeine Skills
7. [ ] Alpha-Tasten und Bewegung: R reden, E aufheben, T Haustier, G Gleiter/Boot, Strg klettern, Shift zielen,
       Bewegungs-Skills (Klettern, Schwimmen, Gleiten, Reiten, Zähmen) nach Skillstufe
8. [ ] Weboberfläche: Skills Klassen zuordnen und duplizieren, Werte live ändern

## Alpha-Referenz

- Fähigkeitsleiste: `alpha/src/cube/GameController.cpp` (`m_barAbilities`, Aktivierung beim Bar-Input)
- Skill-Zuordnung und Boni: `alpha/src/cube/CreatureStats.cpp` (`abilitySkillIndex`, `skillBonus`, `levelBonus`)
- Skillmenü: `alpha/src/cube/widgets/SkillWidget.cpp`, Punktevergabe in `GameController.cpp`
- Ability-IDs: Alpha und Release zählen verschieden (Release meist +1 bis +32); die Zuordnung steht in
  `release/src/mod/AlphaAbilities.cpp`.
