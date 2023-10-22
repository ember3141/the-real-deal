@echo off
del main.o
del main.exe
gcc -c main.cpp -Iinclude -DSFML_STATIC
g++ main.o -o main -Llib -lbox2d -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -lopengl32 -lwinmm -lgdi32 -lfreetype