CC = gcc
TARGET = rawsignal_tx
# CFLAGS: Warnungen, C99-Standard, Pfade zu den Header-Ordnern
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -Iinclude/encoders
# LDFLAGS: Linkt die Mathematik-Bibliothek (-lm) für die sin()-Funktion
LDFLAGS = -lm

# --- Pfade und Verzeichnisse ---
SRC_DIR = src
ENC_DIR = $(SRC_DIR)/encoders
OBJ_DIR = obj

# --- Quelldateien ---
# Liste ALLER C-Dateien, die kompiliert werden sollen (NUR POCSAG)
SRCS = $(SRC_DIR)/rawsignal_tx.c \
           $(SRC_DIR)/signal_generator.c \
           $(ENC_DIR)/pocsag.c

# --- Objektdateien (Vereinfachte und Korrigierte Ableitung) ---
# 1. Ersetze 'src/' durch 'obj/'
OBJ_SRC_FILES = $(patsubst $(SRC_DIR)/%, $(OBJ_DIR)/%, $(SRCS))

# 2. Ersetze '.c' durch '.o'
OBJS = $(OBJ_SRC_FILES:.c=.o)

# Die resultierende Liste OBJS ist jetzt sauber und enthält keine Duplikate.

# --- Hauptziel: Build the executable ---
# Abhängigkeit: Führe das Linken nur aus, wenn alle Objekte existieren
$(TARGET): $(OBJS)
	@echo "Linking target: $@"
	@mkdir -p bin
	$(CC) $(OBJS) -o bin/$@ $(LDFLAGS)


# --- Regeln zum Erstellen der Objektdateien ---

# Ziel: obj/rawsignal_tx.o, obj/signal_generator.o, obj/encoders/pocsag.o, etc.
# Die Regel verwendet die Mustererkennung, um sowohl src/ als auch src/encoders/ zu verarbeiten.

# Beispiel: obj/%.o hängt von src/%.c ab
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Beispiel: obj/src/encoders/%.o hängt von src/encoders/%.c ab
$(OBJ_DIR)/$(ENC_DIR)/%.o: $(ENC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@


# --- Hilfsziele ---
.PHONY: clean run pocsag_test

clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) bin

run: $(TARGET)
	@echo "Running POCSAG test (512 baud):"
	./bin/$(TARGET) POCSAG 512 1234567:3:HELLOTEST | multimon-ng -t raw -a POCSAG512 -

pocsag_test: $(TARGET)
	@echo "Running POCSAG test (0101 pattern):"
	./bin/$(TARGET) POCSAG 1200 1234567:TESTMESSAGE | multimon-ng -t raw -a POCSAG1200 -