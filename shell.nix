
{ pkgs ? import <nixpkgs> {} }:

# This is not all dependencies you need on NixOS
# to build this project.

pkgs.mkShell {
  buildInputs = with pkgs; [
    raylib
    libx11
    glfw
    libGL
    xorg.libXrandr
    xorg.libXinerama
    xorg.libXcursor
    xorg.libXi
  ];
}
