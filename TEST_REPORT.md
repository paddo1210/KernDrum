# Prüfbericht – KernDrum 0.1.0

Stand: 25. Oktober 2026. Lokal entwickelt und gebaut auf Windows 10 x64. JUCE 8.0.12 (`29396c22c93392d6738e021b83196283d6e4d850`), MSVC 19.44.35229 / Visual Studio Build Tools 2022 17.14.42, Windows SDK 10.0.26100, CMake 3.31.8, Ninja 1.12.1. Release-Build, statische MSVC-Laufzeitbibliothek.

## Tatsächlich ausgeführt

| Prüfung | Ergebnis und Umfang |
| --- | --- |
| Nativer Windows-x64-Build | VST3-Bundle und Standalone-Anwendung erfolgreich kompiliert und verlinkt. |
| DSP / Factory-Stimmen | Alle acht Stimmen bei 44,1, 48 und 96 kHz erzeugen Audio; vier Sekunden Ausklang geprüft. Alle geprüften Samples endlich und im begrenzten Ausgangsbereich. Initiale Stille geprüft. |
| Anschlagdynamik | Höhere MIDI-/Trigger-Anschlagstärke erzeugt höhere Signalenergie. |
| Mute / Solo | Mutierte Stimme stumm; Solo einer anderen Stimme schließt nicht-solo geschaltete Stimme aus. |
| Sequencer / Swing | 16 Ereignisse pro vier Viertel bei 60, 123 und 240 BPM sowie Swing 0 / 0,25 / 0,5 / 1. Trigger innerhalb eines Samples der berechneten Grenze. Negativer Vorlauf geprüft. |
| Blockgrenzen | PPQ-Rekonstruktion mit 32, 127, 512 und 2048 Samples: keine doppelten oder ausgelassenen Schritte. Ein anfänglich gefundener Rundungsfehler wurde korrigiert und durch diesen Test abgesichert. |
| MIDI im Processor | Note 36 an Sampleposition 127: davor Stille, danach Audio. Noten 36–43 triggern alle acht Stimmen. |
| Host-Transport im Processor | Simulierter JUCE-PlayHead: Start an PPQ 0, Sprung auf PPQ 2,25, Loop zurück auf 0 und Transportstopp. Die Sequencer-Position folgt korrekt. |
| Automation / Zustand | Normalisierte Parameteränderung wie durch einen Host geprüft. Vollständiger binärer Zustands-Roundtrip in eine neue Processor-Instanz; sämtliche Parameter einschließlich Pattern-Werten und Solo verglichen. Ungültige kurze Zustandsdaten werden ignoriert. |
| Pattern-Bearbeitung | Kopieren eines Patterns einschließlich Schritt 16 von Stimme 8 und anschließendes Löschen geprüft. |
| Editor | Konstruktion sowie Größenänderung auf 960×720 und 1600×1100 geprüft; tatsächliche JUCE-Komponente bei 1120×800 als PNG gerendert und visuell kontrolliert. |
| Tatsächlich gebaute VST3 | Mit dem JUCE-VST3-Host gescannt, als genau ein Instrument erkannt, dynamisch geladen, per MIDI gerendert, Zustand gespeichert/wieder eingelesen und Plugin-Editor erzeugt. Das ist ein Test des Binär-Bundles, nicht nur des DSP-Quellcodes. |

CTest meldet **2 von 2 Tests bestanden** (`CoreTests`, `IntegrationTests`). Der zusätzliche Integrationstest mit dem VST3-Bundle-Pfad meldet ebenfalls PASS. Der DSP-Kern wurde zusätzlich vor dem JUCE-Build unabhängig mit Clang 23.1.1 getestet.

## Lokaler Build-Aufruf

In einer x64-MSVC-Entwicklungsumgebung; `cmake` und `ninja` müssen im Suchpfad liegen:

```powershell
cmake -S outputs/KernDrum -B work/build-native -G Ninja -DCMAKE_BUILD_TYPE=Release -DJUCE_SOURCE_DIR="C:/Pfad/JUCE" -DCMAKE_NINJA_CMCLDEPS_RC=OFF
cmake --build work/build-native --parallel 2
ctest --test-dir work/build-native -C Release --output-on-failure
& ./work/build-native/IntegrationTests_artefacts/Release/IntegrationTests.exe "$PWD/work/build-native/KernDrum_artefacts/Release/VST3/KernDrum.vst3" "$PWD/outputs/KernDrum-UI.png"
```

Der Ninja-Schalter umgeht ein lokal aufgetretenes CMake-Problem beim Windows-Ressourcencompiler. Der normale Visual-Studio-Generator aus der README benötigt diesen Ninja-spezifischen Schalter nicht. MinGW wurde zunächst ausprobiert und von JUCE ausdrücklich abgelehnt; es ist kein unterstützter Build-Weg für diese Ausgabe.

## Offen / nicht als bestanden behauptet

- **Echtes Ableton-Projekt:** Ableton Live 11 Suite ist vorhanden und wurde gestartet. Die Windows-Fensteraufnahme schlug bei Erstversuch und Wiederherstellung mit `FrameArrived timed out` bzw. `window capture timed out` fehl. Deshalb konnten Plugin-Auswahl, Transportbedienung, Automation-Aufzeichnung und Speichern/erneutes Öffnen eines `.als`-Projekts nicht verlässlich über die Oberfläche geprüft werden. Das Plugin wurde im Standardordner `C:\Program Files\Common Files\VST3\KernDrum.vst3` abgelegt. Die oben beschriebenen automatisierten Host-/State-Tests ersetzen keinen solchen DAW-Projekttest.
- Kein Steinberg-Validator-/pluginval-Lauf, kein Test in weiteren DAWs, kein macOS-Build.
- Kein mehrstündiger Dauerlauf, kein systematischer CPU-/Dropout-Benchmark, kein Hardware-MIDI- oder Audiointerface-Test und keine instrumentierte Erfassung sämtlicher Speicheroperationen. Die Echtzeit-Aussagen zum eigenen Callback beruhen auf dem Codeaufbau und den Funktionstests.
- Keine abgeschlossene subjektive Klangabnahme über Lautsprecher/Kopfhörer. Die drei Kit-Ausgangspunkte sind noch keine professionell abgestimmte Preset-Bibliothek.

## Empfohlener kurzer DAW-Abnahmetest

1. KernDrum in einer neuen MIDI-Spur laden. Noten 36–43 mit unterschiedlichen Anschlagstärken senden.
2. DAW starten, stoppen und loopen; Tempo ändern. Pattern-Wechsel und Swing prüfen.
3. Pitch, Decay und Pattern-Auswahl automatisieren; die Wiedergabe abhören.
4. Klangregler und Step-Werte ändern, das DAW-Projekt speichern, schließen und erneut öffnen. Klang, Pattern-Werte und Automation vergleichen.
5. Weitere Instanzen laden und bei typischen Puffergrößen die CPU-/Audioauslastung prüfen.
