from sys import argv
import numpy as np
from PIL import Image

resolution = 100

clusters_map = []

if len(argv) < 2:
    input_name = "output.bin"
else:
    input_name = argv[1]

with open(input_name, "rb") as f:

    entry = f.read(2)

    while entry:
        clusters_map.append(entry[0] + (entry[1]<<8))
        entry = f.read(2)


nb_clusters = len(clusters_map)
chain_length = max(clusters_map)


data = np.zeros((100,resolution), dtype=np.uint8)

slice_size = nb_clusters // resolution
last_slice_size = nb_clusters % resolution

this_slice_size = slice_size

for i in range(resolution):
    mean = 0
    used_clusters = 0

    if (i == resolution - 1) and last_slice_size:
        this_slice_size = last_slice_size

    for j in range(this_slice_size):

        if clusters_map[i*(slice_size) + j] != 0:
            mean += clusters_map[i*(slice_size) + j]
            used_clusters += 1

    if used_clusters:
        mean = int((mean/used_clusters) * (255/chain_length))
    used_clusters = int((used_clusters*100)/this_slice_size)
    if used_clusters == 0:
        used_clusters = 1

    for j in range(used_clusters):
        data[99-j,i] = mean


# Write the bitmap in a png image and show it

img = Image.fromarray(data, 'P')

palette = [0]*256*3
for i in range(256):
    palette[3*i] = i
img.putpalette(palette)

img.save(input_name + '.png')
img.show()
