# Variablen
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -Iinclude/encoders
LDFLAGS = -lm
TARGET = bin/rawsignal_tx

# Quell- und Objektdateien
# Fügt src/encoders/tones.c hinzu
SRC = src/rawsignal_tx.c src/signal_generator.c src/encoders/pocsag.c src/encoders/tones.c

# Fügt obj/encoders/tones.o hinzu
OBJ = $(SRC:src/rawsignal_tx.c=obj/rawsignal_tx.o)
OBJ := $(OBJ:src/signal_generator.c=obj/signal_generator.o)
OBJ := $(OBJ:src/encoders/pocsag.c=obj/encoders/pocsag.o)
OBJ := $(OBJ:src/encoders/tones.c=obj/encoders/tones.o) # NEU: DTMF Objektdatei

# Erstellt eine Liste aller Objektdateien
ALL_OBJ = $(OBJ)


# --- Build-Regeln ---

# Standardregel: Erstellt das Ziel (die ausführbare Datei)
all: directories $(TARGET)
	@echo "Linking target: $(TARGET)"

# Erstellt das ausführbare Ziel
$(TARGET): $(ALL_OBJ)
	$(CC) $(ALL_OBJ) -o $(TARGET) $(LDFLAGS)

# Regel für das Kompilieren von .c zu .o (Generische Regel)
# Stellt sicher, dass das entsprechende Verzeichnis obj/xyz/ existiert
obj/%.o: src/%.c
	@mkdir -p $(@D)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Spezielle Regel für die Encoder-Dateien
obj/encoders/%.o: src/encoders/%.c
	@mkdir -p $(@D)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@


# --- Hilfsregeln ---

# Stellt sicher, dass die Ausgabeordner existieren
directories:
	@mkdir -p bin
	@mkdir -p obj
	@mkdir -p obj/encoders

# Führt das Programm mit einem Testbeispiel aus
run: $(TARGET)
	@echo "Running POCSAG test (512 baud):"
	./$(TARGET) POCSAG 512 1234567:3:FUNKTIONIERT | multimon-ng -t raw -a POCSAG512 -
	@echo "---"
	@echo "Running DTMF test (50ms/50ms):"
	./$(TARGET) DTMF 123456789*#ABC 50 50 > /dev/null
	@echo "DTMF test successfully generated samples (written to stdout)."


# Entfernt alle generierten Dateien
clean:
	@echo "Cleaning up..."
	rm -rf obj bin

.PHONY: all clean run directories