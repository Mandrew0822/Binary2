#!/bin/bash

echo "\033[0;33mCompiling...\n"
cc -o bin2 binary2.c

echo "Moving exeuctable...\n"
sudo mv bin2 /usr/local/bin

cd ..

echo "Done! the Binary2 compiler has been installed to your system"
echo "Try it out by running 'bin2 file.bin2' on some of the example files!"

