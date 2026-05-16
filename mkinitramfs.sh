#!/usr/bin/env bash
# -------------------------------------------------
# mkinitramfs.sh – формирует initramfs с init + busybox
# -------------------------------------------------

set -e

INIT_BIN=./init
BUSYBOX_BIN=${BUSYBOX_BIN:-/bin/busybox}
OUT=initramfs.cpio.gz

rm -f "$OUT"

TMPDIR=$(mktemp -d)

# Создаём структуру директорий
mkdir -p "$TMPDIR"/{bin,sbin,dev,proc,sys,tmp,mnt,etc,root,var/log}

# Копируем init
cp "$INIT_BIN" "$TMPDIR/init"
chmod +x "$TMPDIR/init"

# Копируем busybox и создаём симлинки
if [ -f "$BUSYBOX_BIN" ]; then
    cp "$BUSYBOX_BIN" "$TMPDIR/bin/busybox"
    chmod +x "$TMPDIR/bin/busybox"
    # Создаём симлинки для основных утилит
    for cmd in sh mount umount fdisk mkfs.ext4 mke2fs ls cat cp mv rm \
               mkdir rmdir ln chmod chown grep find echo sleep sync \
               partprobe blkid df du free ps kill uname hostname; do
        ln -sf busybox "$TMPDIR/bin/$cmd"
    done
fi

# Создаём базовые dev nodes
mknod -m 600 "$TMPDIR/dev/console" c 5 1  2>/dev/null || true
mknod -m 666 "$TMPDIR/dev/null"    c 1 3  2>/dev/null || true
mknod -m 666 "$TMPDIR/dev/zero"    c 1 5  2>/dev/null || true
mknod -m 666 "$TMPDIR/dev/tty"     c 5 0  2>/dev/null || true

# Формируем cpio-архив и gzip
(
  cd "$TMPDIR"
  find . -mindepth 1 -printf '%P\0' | cpio --null -ov --format=newc
) | gzip -9 > "$OUT"

rm -rf "$TMPDIR"

echo "Initramfs готов: $OUT ($(du -h "$OUT" | cut -f1))"
