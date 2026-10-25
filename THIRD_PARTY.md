# Abhängigkeiten und Lizenzen

Diese Ausgabe verwendet JUCE 8.0.12, Commit `29396c22c93392d6738e021b83196283d6e4d850`:
https://github.com/juce-framework/JUCE/tree/29396c22c93392d6738e021b83196283d6e4d850

JUCE-Module: AGPLv3 oder kommerzielle JUCE-Lizenz. KernDrum verwendet AGPLv3. Original-Lizenzübersicht:
https://github.com/juce-framework/JUCE/blob/29396c22c93392d6738e021b83196283d6e4d850/LICENSE.md

Die originalen Hinweise und Lizenzdateien sind im vollständigen Quellenpaket unter `JUCE/` enthalten. Das Binärpaket enthält zusätzlich den Ordner `Licenses` mit den aus dem verwendeten JUCE-Baum übernommenen Lizenztexten. Das Quellenpaket ist mit derselben Version wie das Binärpaket im GitHub-Release verfügbar. Der Anwendungsquellcode ist im Repository enthalten; CMake lädt genau den oben festgelegten JUCE-Commit.

Wesentliche enthaltene Abhängigkeiten (maßgeblich sind die Originaltexte in JUCE):

- VST3-SDK: MIT, Copyright 2025 Steinberg Media Technologies GmbH.
- HarfBuzz: MIT-artige Lizenz.
- SheenBidi: Apache 2.0.
- libpng und zlib: zlib-Lizenzen.
- JPEG: Independent JPEG Group.
- FLAC: BSD-artige Lizenz.
- Ogg/Vorbis: BSD-artige Lizenzen.

ASIO, AAX, AU, LV2 und Web-Browser-Integration werden für diese Windows-Ausgabe nicht aktiviert. Microsoft Build Tools und Windows SDK dienen nur zum Bauen und werden nicht weitergegeben. CMake (BSD 3-Clause), Ninja (Apache 2.0) und LLVM (Apache 2.0 mit LLVM-Ausnahmen) sind ebenfalls reine Build-Werkzeuge und kein Bestandteil der Auslieferung.

Es wurden keine Sonic-Charge-Dateien, Microtonic-Presets, Samples, Grafiken oder Quelltexte verwendet. Es besteht keine Verbindung zu Sonic Charge oder Steinberg.
