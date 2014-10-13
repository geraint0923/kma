#/bin/bash

echo "Evaluating Test Cases on VM";
cd /home/aqualab/.aqualab/project2;
make test-reg SHELL_ARCH=64;
