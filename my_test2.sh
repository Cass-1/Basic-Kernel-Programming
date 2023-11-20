#!/bin/sh

sudo rmmod kmlab
make
sudo insmod kmlab.ko
./userapp 10 &
./userapp 15 &