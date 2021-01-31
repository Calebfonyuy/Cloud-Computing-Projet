#!/bin/bash

# This will most likely not be the same for you so change it
path_to_qemu=~/Documents/qemu


source snap_number

# Update the number of the current snapshot
new_snap_nb=$(($snap_nb + 1))
echo "snap_nb=$new_snap_nb" > snap_number

# Create the snapshot. The -b option indicates the backing file for the file being created.
$path_to_qemu/build/qemu-img create -f qcow2 -b sample_disk$snap_nb.qcow2 sample_disk$new_snap_nb.qcow2
