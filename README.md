# rawsignal-tx: Universal RF Signal Generator (POCSAG & DTMF & ...)

## 📡 Übersicht

`rawsignal-tx` ist ein Kommandozeilen-Tool, das zur Generierung von rohen, digitalen Audiosignalen für verschiedene Funkprotokolle entwickelt wurde. Die Ausgabe erfolgt als **Signed 16-bit Little-Endian (S16_LE) PCM Audio** mit einer Abtastrate von **22050 Hz** und wird direkt an `stdout` ausgegeben.

---

## ✨ Unterstützte Modulatoren

### 1. POCSAG Paging Protocol

POCSAG (Post Office Code Standardisation Advisory Group) ist ein Standard-Protokoll für Funkrufempfänger. Der Encoder generiert POCSAG-konforme Bursts, die Adressen, Funktionscodes und alphanumerische Nachrichten enthalten.

* **Implementierung:** **Robuste Rechteckwellen-FSK** (aktuell).
* **Unterstützte Baudraten:** 512, 1200, 2400.

### 2. DTMF (Dual-Tone Multi-Frequency)

DTMF, bekannt als **Tastenwahl-Signalisierung** (Touch-Tone), generiert Töne durch die Überlagerung von zwei Sinuswellen.

* **Implementierung:** Reines Sinuswellen-Tonsignal.
* **Unterstützte Zeichen:** 0-9, \*, \#, A, B, C, D.

---

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

-----

## 🚀 Nutzung

Das Programm benötigt immer mindestens einen Modulator und spezifische Parameter.

### 1\. POCSAG-Nutzung

Generiert ein POCSAG-Signal.

#### Syntax

```bash
./bin/rawsignal_tx POCSAG <BAUD> <ADRESSE>:<FUNKTION>:<NACHRICHT>
```

  * `<BAUD>`: 512, 1200, oder 2400.
  * `<ADRESSE>`: Bis zu 21-Bit-Adresse.
  * `<FUNKTION>`: 0-3 (z.B. 3 für Alpha-Nachricht). Kann weggelassen werden (Standard ist 3).

#### Beispiel: Generiere POCSAG und dekodiere mit multimon-ng

```bash
# Nachricht an Adresse 1234567, 512 Baud
./bin/rawsignal_tx POCSAG 512 1234567:3:HALLO TEST | multimon-ng -t raw -a POCSAG512 -
```

### 2\. DTMF-Nutzung

Generiert eine Sequenz von DTMF-Tönen.

#### Syntax

```bash
./bin/rawsignal_tx DTMF <SEQUENZ> <TON_DAUER_MS> <PAUSE_DAUER_MS>
```

  * `<SEQUENZ>`: Die zu sendende Zeichenkette (z.B. `123456*#A`).
  * `<TON_DAUER_MS>`: Dauer jedes Tons in Millisekunden (Standard: 50).
  * `<PAUSE_DAUER_MS>`: Dauer der Stille zwischen den Tönen in Millisekunden (Standard: 50).

#### Beispiel: Generiere DTMF und dekodiere mit multimon-ng

```bash
# Sequenz 123456*#A, 50ms Ton, 50ms Pause
./bin/rawsignal_tx DTMF 123456*#A 50 50 | multimon-ng -t raw -a DTMF -
```

#### Beispiel: Akustische Ausgabe (setzt `aplay` voraus)

```bash
./bin/rawsignal_tx DTMF 5551234 80 80 | aplay -r 22050 -f S16_LE
```

-----

## 🧪 Entwicklungsstand

Der Code wurde erfolgreich implementiert und die DTMF-Funktionalität mit `multimon-ng` verifiziert.

### Nächste geplante Schritte

Implementierung weiterer Protokolle (z.B. FLEX).
