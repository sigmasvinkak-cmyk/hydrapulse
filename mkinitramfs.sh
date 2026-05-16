#!/usr/bin/env bash
# -------------------------------------------------
# mkinitramfs.sh – формирует initramfs с нашим init
# -------------------------------------------------

set -e

INIT_BIN=./init            # путь к собранному init
OUT=initramfs.cpio.gz

# Удаляем старый образ, если существует
rm -f "$OUT"

# Временная директория для создания образа
TMPDIR=$(mktemp -d)

# Копируем бинарник и делаем его исполняемым
cp "$INIT_BIN" "$TMPDIR/init"
chmod +x "$TMPDIR/init"

# Формируем cpio‑архив (newc) и сразу gzip‑им
(
  cd "$TMPDIR"
  find . -mindepth 1 -printf '%P\0' | cpio --null -ov --format=newc
) | gzip -9 > "$OUT"

# Убираем временные файлы
rm -rf "$TMPDIR"

echo "Initramfs готов: $OUT"
