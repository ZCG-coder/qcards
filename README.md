# qcards
Quick reference flashcards

To build:
```sh
cc qcards.c -o qcards
```

On macOS (universal binary):
```sh
arch -x86_64 cc qcards.c -o qcards.x86
arch -arm64 cc qcards.c -o qcards.arm64
lipo -create -output qcards qcards.x86 qcards.arm64
```
