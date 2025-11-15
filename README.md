# rawsignal-tx: Universal RF Signal Generator

## 📡 Übersicht

`rawsignal-tx` ist ein Kommandozeilen-Tool zur Generierung von rohen, digitalen Audiosignalen für verschiedene Funkprotokolle. Die Ausgabe erfolgt als **Signed 16-bit Little-Endian (S16_LE) PCM Audio** mit einer Abtastrate von **22050 Hz** und wird direkt an `stdout` ausgegeben.

---

## ✨ Unterstützte Modulatoren

| Modulator | Status | Dekodierung | Anwendung |
|-----------|--------|------------|-----------|
| **POCSAG** | ✅ Funktioniert | multimon-ng | Funkrufempfänger |
| **DTMF** | ✅ Funktioniert | multimon-ng / aplay | Tastenwahl-Signalisierung |
| **MORSE_CW** | ✅ Funktioniert | multimon-ng | Morsecode |
| **UFSK1200** | ⚠️ Experimentell | multimon-ng (teilweise) | Einfache FSK-Modulation |
| **FSK9600** | ⚠️ Experimentell | multimon-ng (nein) | Hochgeschwindigkeit FSK |
| **AFSK1200** | ❌ Nicht funktional | multimon-ng (nein) | AX.25 APRS |

## 🛠️ Build-Anleitung

Das Projekt basiert auf C99 und wird mithilfe eines `Makefile` kompiliert.

### Voraussetzungen

Sie benötigen einen C-Compiler (`gcc`) und die Standard-Build-Tools (`make`).

```bash
# Auf Debian/Ubuntu-Systemen (z.B. Raspberry Pi OS)
sudo apt update
sudo apt install build-essential
````

### Kompilieren

Navigieren Sie in das Hauptverzeichnis des Projekts und führen Sie `make` aus:

```bash
make
```

Die ausführbare Datei wird im Verzeichnis `bin/rawsignal_tx` erstellt.

### Aufräumen

Zum Entfernen aller generierten Objektdateien und der ausführbaren Datei:

```bash
make clean
```

## 🚀 Nutzung

Das Programm benötigt immer mindestens einen Modulator und spezifische Parameter.

### POCSAG

```bash
./bin/rawsignal_tx POCSAG <BAUD> <ADRESSE>:<FUNKTION>:<NACHRICHT>
```

**Beispiel:**
```bash
./bin/rawsignal_tx POCSAG 512 "1234567:3:HALLO TEST" | multimon-ng -t raw -a POCSAG512 -
```

### DTMF

```bash
./bin/rawsignal_tx DTMF <SEQUENZ> [TON_MS] [PAUSE_MS]
```

**Beispiele:**
```bash
# Mit multimon-ng dekodieren
./bin/rawsignal_tx DTMF 123456*#A 50 50 | multimon-ng -t raw -a DTMF -

# Mit Lautsprechern abspielen
./bin/rawsignal_tx DTMF 5551234 80 80 | aplay -r 22050 -f S16_LE
```

### MORSE_CW

```bash
./bin/rawsignal_tx MORSE_CW "<NACHRICHT>" [WPM]
```

**Beispiel:**
```bash
./bin/rawsignal_tx MORSE_CW "HELLO WORLD" 20 | multimon-ng -a MORSE_CW -
```

### UFSK1200 (experimentell)

```bash
./bin/rawsignal_tx UFSK1200 "<NACHRICHT>"
```

**Beispiel:**
```bash
./bin/rawsignal_tx UFSK1200 "Test" | multimon-ng -t raw -a UFSK1200 -
```

**Hinweis:** UFSK1200 zeigt begrenzte Dekodierung durch multimon-ng (teilweise funktional, wahrscheinlich Sample-Rate-Timing-Probleme).

### FSK9600 (experimentell)### FSK9600 (experimentell)

```bash
./bin/rawsignal_tx FSK9600 "<NACHRICHT>"
```

**Beispiel:**
```bash
./bin/rawsignal_tx FSK9600 "Test" | multimon-ng -t raw -a FSK9600 -
```

**Hinweis:** FSK9600 generiert Audio, wird aber von multimon-ng nicht dekodiert.

-----

## 🧪 Entwicklungsstand

| Feature | Status |
|---------|--------|
| POCSAG Encoder | ✅ Vollständig funktioniert |
| DTMF Encoder | ✅ Vollständig funktioniert |
| MORSE_CW Encoder | ✅ Vollständig funktioniert |
| UFSK1200 Encoder | ⚠️ Teilweise funktional |
| FSK9600 Encoder | ⚠️ Audio generiert, nicht dekodiert |
| AFSK1200 Encoder | ❌ Nicht funktional (siehe Probleme) |

### Bekannte Probleme

**AFSK1200:** Generiert gültiges PCM-Audio, wird aber von multimon-ng nicht dekodiert. Mögliche Ursachen:
- Frame-Struktur nicht AX.25-konform
- NRZI-Encoding-Logik
- Bit-Stuffing bei Flaggen

**UFSK1200 & FSK9600:** Timing-Probleme bei 22050 Hz Abtastrate (nicht exakt teilbar durch Baudrate).

-----

## 📚 Projektstruktur

```
rawsignal-tx/
├── Makefile                    # Build-System
├── README.md                   # Dokumentation
├── .github/
│   └── copilot-instructions.md # AI Agent Anleitung
├── include/
│   ├── signal_generator.h      # PCM-Erzeugung
│   ├── crc.h                   # CRC16-CCITT Utility
│   └── encoders/
│       ├── pocsag.h
│       ├── tones.h
│       ├── morse.h
│       ├── afsk1200.h
│       ├── ufsk1200.h
│       └── fsk9600.h
└── src/
    ├── rawsignal_tx.c          # CLI & Hauptprogramm
    ├── signal_generator.c      # PCM-Erzeugung
    ├── crc.c                   # CRC16-CCITT
    └── encoders/
        ├── pocsag.c
        ├── tones.c
        ├── morse.c
        ├── afsk1200.c
        ├── ufsk1200.c
        └── fsk9600.c
```

## 🙏 Danksagungen

Der POCSAG-Encoder basiert auf der Implementierung von **[`faithanalog/pocsag-encoder`](https://github.com/faithanalog/pocsag-encoder)**.
