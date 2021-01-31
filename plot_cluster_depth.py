from sys import argv
import numpy as np
from matplotlib import pyplot as plt
import seaborn as sns

data_src = "./clusters_depth.log"
file_name = "output.bin"

if len(argv)>1:
    file_name = argv[1]

file = open(data_src)
text = file.read()
if text.endswith('\n'):
    text = text[:-1]
data = [int(i) for i in text]

data_arr = np.array(data)
absx = np.arange(len(data_arr))

plt.figure(figsize=(10,10))
ax = sns.barplot( data_arr, absx)
plt.savefig("./simulation/files/histograms/"+file_name+".png")