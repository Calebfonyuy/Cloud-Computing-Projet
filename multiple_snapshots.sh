#!/bin/bash

# This will most likely not be the same for you so change it
path_to_qemu=~/Documents/qemu

source snap_number

# Start the VM with the iso as a startup CD
$path_to_qemu/build/qemu-system-x86_64 \
    -enable-kvm \
    -m 2048 \
    -nic user,model=virtio \
    -drive file=sample_disk$snap_nb.qcow2,media=disk,if=virtio \
    -display gtk \
    -L $path_to_qemu/pc-bios&

# Sleep for 3 minutes
sleep 180

# Loop to take snapshots and treat

while true
do
# Take snapshot of VM
./snapshot.sh

source snap_number

./clusters_map/clusters_map ./sample_disk$snap_nb.qcow2 output_$snap_nb.bin

python3 ./plot_clusters_depth.py output_$snap_nb.bin

python3 ./read_write.py

# sleep for two minutes
sleep 120
done
