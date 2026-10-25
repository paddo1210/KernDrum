# KernDrum 0.1.0

Eigenständiges synthetisches Drum-Instrument für Windows x64: acht Stimmen, vier Patterns mit je acht Spuren und 16 Steps, VST3 und zusätzliche Standalone-Anwendung. Eigener Code, eigener Name und eigene Oberfläche. Die Idee einer direkt bedienbaren Drum-Synthese mit Pattern-Sequencer ist von [Microtonic](https://soniccharge.com/microtonic) inspiriert; KernDrum verwendet keine Sounds, Grafiken oder Algorithmen dieses Produkts und ist kein klanggetreuer Nachbau.

## Installation

Das Windows-Release herunterladen und entpacken. Den **gesamten Ordner `KernDrum.vst3`**, einschließlich `Contents`, in einen von der DAW gescannten VST3-Ordner kopieren. Üblicher systemweiter Pfad: `C:\Program Files\Common Files\VST3` (kann Administratorrechte benötigen). Alternativ einen eigenen VST3-Suchpfad in der DAW verwenden. Danach Plugins neu scannen und KernDrum auf einer MIDI-Spur als Instrument laden.

Es gibt einen Stereo-Ausgang und keinen Audio-Eingang. Die Standalone-Datei ist zum manuellen Anspielen gedacht; ihr interner Sequencer benötigt die Transport-/PPQ-Informationen einer DAW und läuft dort nicht frei.

## Bedienung

- **MIDI:** Notennummern **36 bis 43**, auf allen Kanälen, spielen die acht Stimmen. Die angezeigten Oktavnamen unterscheiden sich zwischen DAWs. Die Stimmen sind monophone One-Shots; Note-Off verkürzt den Klang nicht. Anschlagstärke steuert die Lautstärke vor der Verzerrung. CC120/123 stoppt ausklingende Stimmen.
- **Stimmenwahl / Hit:** links eine Stimme auswählen; „Hit“ spielt sie zum Vorhören. Standardstimmen: Kick, Snare, Clap, Closed Hat, Open Hat, Low Tom, High Tom, Metal.
- **Klang:** Pitch in Hz, Pitch env in Halbtönen (auch negativ), Pitch time und Decay in ms, Noise mix, Noise filter in Hz, Drive, Level, Pan. High-pass noise wechselt zwischen Tief- und Hochpass. Triangle tone blendet auf eine Dreiecksschwingung. Clap pulses ergänzt drei um 11 ms versetzte Hüllkurvenimpulse. Mute und Solo wirken pro Stimme; mehrere Solos sind möglich. Mute hat Vorrang.
- **Sequencer:** DAW starten und „Sequencer“ einschalten. Ein Pattern ist ein 4/4-Takt aus 16 Sechzehnteln, unabhängig von der Taktart des Hosts. Klick schaltet einen Step an/aus; Rechtsklick setzt volle Anschlagstärke. Nach oben/unten ziehen verändert die Anschlagstärke. Die kleine Linie im Step zeigt ihren Wert.
- **Swing:** 0 = gerade; 1 verzögert jeden zweiten Sechzehntel um eine Achtel-Sechzehntel-Paarhälfte, also 1/8 Viertelnote. Die Paarlänge bleibt gleich. Der Regler ist bewusst als Wert 0–1 dargestellt, nicht als klassischer Swing-Prozentsatz.
- **Patterns 1–4:** Auswahl schaltet im nächsten Audioblock um, ohne die Taktposition zurückzusetzen. „Copy to next“ überschreibt das nächste Pattern und wählt es an (4 → 1). „Clear pattern“ leert das gewählte Pattern. Diese Aktionen sind sofort wirksam.
- **Kits:** Core, Weight, Spark im Menü wählen und „Load kit“ drücken. Dies ersetzt alle acht Klang-Einstellungen einschließlich Mute/Solo; Patterns bleiben erhalten. Pattern 1 enthält beim ersten Laden einen Ausgangsgroove, 2–4 sind leer.
- **Output / Pegel:** Ausgangslautstärke oben rechts; zwei Peak-Anzeigen. Weiche Sättigung begrenzt den Ausgang unter 0 dBFS. Regler doppelklicken zum Ausgangswert der jeweiligen Stimme. Fenstergröße an den Rändern verändern.
- **Speichern / Automation:** alle Klangparameter, Mute/Solo, Master, Swing, Sequencer an/aus, Pattern-Auswahl und alle 512 Step-Anschlagstärken werden als Host-Parameter gespeichert und können automatisiert werden. Das DAW-Projekt speichert den vollständigen musikalischen Zustand. Fenstergröße und zuletzt gewählte Edit-Stimme sind reine Ansichtseinstellungen.

## Reproduzierbar bauen

Benötigt: Git, CMake ≥ 3.22, Visual Studio 2022 Build Tools mit **MSVC x64/x86** und **Windows SDK**. Unter Windows ist MinGW von JUCE 8.0.12 ausdrücklich nicht unterstützt. Das Projekt benötigt kein Projucer und kein zusätzliches VST-SDK. CMake holt JUCE beim ersten Konfigurieren aus dem Netz, festgelegt auf Commit `29396c22c93392d6738e021b83196283d6e4d850` (8.0.12).

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
& ./build/IntegrationTests_artefacts/Release/IntegrationTests.exe "$PWD/build/KernDrum_artefacts/Release/VST3/KernDrum.vst3"
```

Ergebnis: `build/KernDrum_artefacts/Release/VST3/KernDrum.vst3` und `build/KernDrum_artefacts/Release/Standalone/KernDrum.exe`. Zum Bauen ohne Netz das vollständige Quellenarchiv einschließlich JUCE entpacken und beim Konfigurieren `-DJUCE_SOURCE_DIR="C:/Pfad/JUCE"` ergänzen. Das Verzeichnis muss die zugehörige JUCE-Version enthalten. Build- und Quellordner getrennt halten.

Auf macOS mit Xcode-Command-Line-Tools und CMake ist dieselbe Architektur verwendbar; z. B. `cmake -S . -B build -G Xcode`, danach dieselben Build-/CTest-Befehle. macOS-Build, Universal-Binary, Signierung und Notarisierung wurden hier nicht durchgeführt. AU ist derzeit kein Build-Ziel.

## Aufbau und Echtzeitverhalten

`Source/Engine.h` enthält den unabhängigen DSP-Kern; `Processor.*` bindet MIDI, PPQ-Transport, Parameter und Zustand an JUCE; `Editor.*` bildet die Oberfläche. `Tests/CoreTests.cpp` prüft DSP und Raster; `Tests/IntegrationTests.cpp` den Processor und optional die tatsächlich gebaute VST3 über den JUCE-VST3-Host.

Pro Stimme: Sinus/Dreieck, eigener deterministischer Xorshift-Rauschgenerator, Tonhöhen- und Lautstärkehüllkurven, einfacher Rauschfilter, weiche Verzerrung und Stereo-Panorama. Kurzer Attack und ein abklingender Restwert entschärfen Retrigger. Kontinuierliche Parameter und Filter-/Hüllkurvenkoeffizienten sind geglättet. Jede Stimme verarbeitet auch im Mute ihren Ausklang. Der Audiopfad verwendet feste Arrays und zwischengespeicherte atomare Parameteradressen; keine eigenen Sperren, Dateizugriffe, GUI-Aufrufe oder dynamischen Speicherreservierungen im Audio-Callback. Zustandssicherung und GUI-Änderungen liegen außerhalb dieses Pfads. Parameteränderungen werden pro Audioblock übernommen, MIDI-Ereignisse und Sequencer-Trigger innerhalb des Blocks samplegenau verarbeitet.

Die Transportposition wird je Audioblock direkt vom Host übernommen. Tempoänderungen wirken mit dessen nächstem Tempo-/PPQ-Update. Sprünge und Loops folgen der Hostposition; ein Start mitten in einem Step erzeugt erst an der nächsten Grenze einen Sequencer-Trigger. Bei fehlender PPQ-Position bleibt die Sequenz stumm; MIDI und Hit funktionieren weiter. Laufende Klangfahnen dürfen beim Transportstopp ausklingen.

## Grenzen und nächste Erweiterungen

Version 0.1 ist eine nutzbare Grundversion, kein Ersatz für sämtliche Funktionen von Microtonic. Es gibt kein Morphing, keine Zufallsgeneratoren, kein Pattern-Chaining, keine Einzelausgänge, keinen MIDI-Export und kein Dateimenü für Kit-Dateien. Die vier Patterns und drei Kit-Ausgangspunkte sind bewusst überschaubar. Kopieren/Löschen/Kit-Laden hat noch kein internes Undo. Stimmen sind einzeln monophon und werden bei erneutem Trigger neu gestartet. Hats besitzen keine automatische Choke-Gruppe. Verzerrung und Dreieck sind nicht oversampled; bei hohen Tonhöhen und starkem Drive kann Aliasing Teil des Klangs sein. Der Noise-Filter ist ein einfacher Einpol-Filter ohne Resonanz. Factory-Kits sind technische Ausgangspunkte und noch keine professionell kuratierte Soundbank.

Nächste Schritte: weitere Klangpresets und Hörabstimmung, Bandbegrenzung/Oversampling, Choke-Gruppen, Undo, Pattern-Wechsel am Taktanfang, interne Standalone-Clock, Einzelausgänge und macOS-Auslieferung. Durchgeführte und offene Prüfungen stehen in [TEST_REPORT.md](TEST_REPORT.md).

## Lizenz

KernDrum wird als **AGPL-3.0-only** mit vollständigem Quellcode bereitgestellt. JUCE 8.0.12 ist dual lizenziert: AGPLv3 oder kommerzielle JUCE-Lizenz; diese Ausgabe verwendet den AGPL-Weg. Das in genau dieser JUCE-Version enthaltene VST3-SDK steht unter MIT. Für Weitergabe und eigene Änderungen gelten die jeweiligen Lizenzpflichten. Die Namen VST und VST3 sind Marken von Steinberg. Siehe [LICENSE](LICENSE) und [THIRD_PARTY.md](THIRD_PARTY.md). Ein geschlossenes kommerzielles Folgeprodukt benötigt eine passende Rechte- und Lizenzklärung.
