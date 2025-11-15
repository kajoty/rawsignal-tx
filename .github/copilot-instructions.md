## Kurzüberblick

Dieses Repository ist ein kleines C-Tool zur Erzeugung roher PCM-Audiosignale (S16_LE, 22050 Hz) für mehrere Funkprotokolle (z.B. POCSAG, DTMF, Morse, AFSK1200).

**Wichtig:** Die Binärdatei `bin/rawsignal_tx` schreibt das Audiosignal direkt auf `stdout` (Signed 16-bit Little-Endian, `S16_LE`). Viele Workflows erwarten eine Weiterleitung an `aplay` oder ein Demodulator-Tool wie `multimon-ng`.

## Architektur & wichtige Komponenten

- `src/rawsignal_tx.c`: CLI, Auswahl der Modulatoren, Argument-Parsing und Steuerung des Workflows.
- `src/signal_generator.c` + `include/signal_generator.h`: gemeinsame Hilfsfunktionen zur PCM-Erzeugung (z.B. `pcmTransmissionLength`, `pcmEncodeTransmission`, `rs_generate_tone_sample`).
- `src/crc.c` + `include/crc.h`: CRC-Utilities (POCSAG-relevant).
- `src/encoders/*.c` + `include/encoders/*.h`: Protokoll-spezifische Encoder (POCSAG, DTMF/tones, MORSE, AFSK1200). Suche nach `pocsag_`, `morse_`, `rs_encode_dtmf`, `rs_encode_afsk1200`-Symbolen.

Design-Intent:
- Encoder erzeugen entweder ein Array von 32-bit-Wörtern (z.B. POCSAG) oder schreiben direkt PCM-Samples an `stdout` (z.B. DTMF-Encoder). Achte auf die Funktion, die verwendet wird: `pocsag_encodeTransmission` → Konvertierung → `pcmEncodeTransmission`, vs. `rs_encode_dtmf` schreibt direkt.

## Build / Run / Debug

- Build: `make` (Erzeugt `bin/rawsignal_tx`). Flags in `Makefile`: `-std=c99 -Wall -Wextra -O2 -Iinclude`, Link mit `-lm`.
- Clean: `make clean`.
- Run examples (aus README):
  - POCSAG (512 Baud): `./bin/rawsignal_tx POCSAG 512 "1234567:3:HALLO TEST" | multimon-ng -t raw -a POCSAG512 -`
  - DTMF to audio: `./bin/rawsignal_tx DTMF 123456*#A 50 50 | aplay -r 22050 -f S16_LE`
  - MORSE_CW: `./bin/rawsignal_tx MORSE_CW "HELLO" 20 | multimon-ng -a MORSE_CW -`

Tipps zum Debugging:
- Da Ausgaben an `stdout` geschrieben werden, redirect auf Dateien (`> out.raw`) oder an `aplay`/`multimon-ng`.
- Viele Encoder verwenden dynamische Allokation. Valgrind ist hier nützlich: `valgrind --leak-check=full ./bin/rawsignal_tx ...`.

## Konventionen & Fallen

- C-Standard: C99. Halte dich an `include/`-Headers für Deklarationen.
- Namenskonventionen: Protokoll-spezifische APIs verwenden Präfixe (`pocsag_`, `morse_`, `rs_encode_*`). Wenn du neue Encoder hinzufügst, folge diesem Muster und lege Header in `include/encoders/` ab.
- PCM-Längen: Achte auf Einheiten — Funktionen wie `pcmTransmissionLength` geben Sample-Anzahlen; Buffer-Größen werden oft in Samples oder Bytes gerechnet. Überprüfe `sizeof(int16_t)`-Multiplikationen.
- Ausgabeformat: Immer `S16_LE`, `SAMPLE_RATE` ist 22050 (`include/signal_generator.h`).
- Keine externen Laufzeit-Abhängigkeiten hinzugefügt ohne Rücksprache — das Projekt ist auf kleine, direkte C-Implementationen ausgelegt.
- **Known Issue – AFSK1200:** Der AFSK1200-Encoder generiert gültige PCM-Audio, wird aber von `multimon-ng` nicht dekodiert. Mögliche Ursachen: Frame-Struktur, NRZI-Zustand, oder Bit-Stuffing-Logik. POCSAG, MORSE_CW und DTMF funktionieren korrekt.

## Beispiele aus dem Code (so suchen/lesen)
- CLI-Logik in `src/rawsignal_tx.c` zeigt erlaubte Modulatoren und Argument-Formate.
- PCM-Erzeugung: `pcmEncodeTransmission(...)` in `include/signal_generator.h` / `src/signal_generator.c`.
- POCSAG-Flow: `pocsag_messageLength` → `pocsag_encodeTransmission` → `pcmEncodeTransmission`.

## Bearbeitungsrichtlinien für KI-Agenten

- PR-Größe: Bevorzuge kleine, zielgerichtete Änderungen (ein Encoder / Bugfix pro PR).
- Tests: Es gibt keine automatisierten Tests im Repo; füge reproduzierbare CLI-Beispiele in die PR-Description.
- Ressourcen: Nutze `README.md`, `Makefile` und die Header in `include/` als primäre Quellen für Annahmen.

Wenn etwas unklar ist, frage konkret nach: z.B. "Welche erwartete Ausgabe (Samples/Bytes) hat X-Funktion?" oder "Soll neues Encoder-API direkt PCM schreiben oder 32-bit-Wörter zurückgeben?" 

---
Bitte sag Bescheid, welche Bereiche ich detaillierter beschreiben oder mit Code-Beispielen ergänzen soll.
