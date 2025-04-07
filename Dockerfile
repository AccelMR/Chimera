FROM archlinux:latest

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm base-devel git cmake mingw-w64-gcc wine

WORKDIR /src