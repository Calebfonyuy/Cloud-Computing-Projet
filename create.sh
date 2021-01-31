#!/bin/bash

# This will most likely not be the same for you so change it
path_to_qemu=~/Documents/qemu

# Get an iso
# curl -O https://releases.ubuntu.com/20.04/ubuntu-20.04.1-live-server-amd64.iso

# Create empty qcow2 file and write its name in a file
$path_to_qemu/build/qemu-img create -f qcow2 sample_disk0.qcow2 16G

# This is one way to keep track of the number of the current snapshot
echo snap_nb="0" > snap_number

# Start the VM with the iso as a startup CD
$path_to_qemu/build/qemu-system-x86_64 \
    -enable-kvm \
    -m 2048 \
    -nic user,model=virtio \
    -drive file=sample_disk0.qcow2,media=disk,if=virtio \
    -display gtk \
    -cdrom ubuntu-20.04.1-live-server-amd64.iso \
    -L $path_to_qemu/pc-bios
