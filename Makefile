# Compiler und Flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -Iinclude
LDFLAGS = -lm # -lm ist für mathematische Funktionen (z.B. sin, cos)

# Verzeichnisse
BIN_DIR = bin
OBJ_DIR = obj
SRC_DIR = src
MOD_SRC_DIR = src/encoders

# Ausgabedatei
TARGET = $(BIN_DIR)/rawsignal_tx

# --- Quellendateien ---

# Haupt-Quellendateien (ohne CRC)
CORE_SRCS = $(SRC_DIR)/rawsignal_tx.c \
            $(SRC_DIR)/signal_generator.c

# CRC-Datei
CRC_SRC = $(SRC_DIR)/crc.c

# Modulator-Quellendateien
MODULATOR_SRCS = $(MOD_SRC_DIR)/pocsag.c \
                 $(MOD_SRC_DIR)/tones.c \
                 $(MOD_SRC_DIR)/morse.c \
                 $(MOD_SRC_DIR)/afsk1200.c

# Alle Quellendateien
SRCS = $(CORE_SRCS) $(CRC_SRC) $(MODULATOR_SRCS)


# --- Objektdateien ---

CORE_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CORE_SRCS))
CRC_OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CRC_SRC))
MODULATOR_OBJS = $(patsubst $(MOD_SRC_DIR)/%.c, $(OBJ_DIR)/encoders/%.o, $(MODULATOR_SRCS))

# Alle Objektdateien
OBJS = $(CORE_OBJS) $(CRC_OBJ) $(MODULATOR_OBJS)

# --- Regeln ---

.PHONY: all clean

all: $(BIN_DIR) $(OBJ_DIR)/encoders $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Regel für alle .c-Dateien in src/ (CRC.c wird hier auch kompiliert)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Regel für .c-Dateien in src/encoders/
$(OBJ_DIR)/encoders/%.o: $(MOD_SRC_DIR)/%.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Erstelle Verzeichnisse
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR)/encoders:
	mkdir -p $(OBJ_DIR)/encoders

clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)