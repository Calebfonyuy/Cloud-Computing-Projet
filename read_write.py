from datetime import datetime
import pandas as pd
import os

read = pd.read_csv('log_read.log', sep=',', header=None, names = ["date", "heure", "durée", "offset", "cur_bytes", "bytes", "cluster"])
temps = read["durée"].str.split(" : ",  n = 0, expand = True)
read["durée"]= temps[1]

offset = read["offset"].str.split(" : ",  n = 0, expand = True)
read["offset"]= offset[1]

cur_bytes = read["cur_bytes"].str.split(" : ",  n = 0, expand = True)
read["cur_bytes"]= cur_bytes[1] 

byte = read["bytes"].str.split(" : ",  n = 0, expand = True)
read["bytes"]= byte[1]

read["offset"] = read["offset"].astype(int) 
read["cluster"] = (read["offset"] // 65536)*65536

debut = read["date"][0].split("-") + read["heure"][0].split(":")
debut = datetime(int(debut[0]), int(debut[1]), int(debut[2]), int(debut[3]), int(debut[4]), int(debut[5]))

fin = read["date"][len(read["date"])-1].split("-") + read["heure"][len(read["date"])-1].split(":")
fin = datetime(int(fin[0]), int(fin[1]), int(fin[2]), int(fin[3]), int(fin[4]), int(fin[5]))

difference = (fin - debut).seconds

read["cur_bytes"] = read["cur_bytes"].astype(int) 
read["bytes"] = read["bytes"].astype(int) 
read["durée"] = read["durée"].astype(float) 

new_read = read.groupby("cluster").sum()

new_read["nb_occurence"] = ( read.groupby("cluster").count() )["durée"]
new_read = new_read.drop("offset", axis=1)

new_read = new_read.reset_index()
new_read["read_intensity(ops)"] = new_read["nb_occurence"]/difference
new_read["débit(o/s)"] = new_read["cur_bytes"]/new_read["durée"]
#new_read = new_read.T
if os.path.exists('./simulation/files/json/read_log.json'):
	os.remove('./simulation/files/json/read_log.json')
new_read.to_json('./simulation/files/json/read_log.json', orient='records', force_ascii=False)

write = pd.read_csv('log_write.log', sep=',', header=None, names = ["date", "heure", "durée", "offset", "cur_bytes", "bytes", "cluster"])
temps = write["durée"].str.split(" : ",  n = 0, expand = True)
write["durée"]= temps[1]

offset = write["offset"].str.split(" : ",  n = 0, expand = True)
write["offset"]= offset[1]

cur_bytes = write["cur_bytes"].str.split(" : ",  n = 0, expand = True)
write["cur_bytes"]= cur_bytes[1] 

byte = write["bytes"].str.split(" : ",  n = 0, expand = True)
write["bytes"]= byte[1]

write["offset"] = write["offset"].astype(int) 
write["cluster"] = (write["offset"] // 65536)*65536

debut = write["date"][0].split("-") + write["heure"][0].split(":")
debut = datetime(int(debut[0]), int(debut[1]), int(debut[2]), int(debut[3]), int(debut[4]), int(debut[5]))

fin = write["date"][len(write["date"])-1].split("-") + write["heure"][len(write["date"])-1].split(":")
fin = datetime(int(fin[0]), int(fin[1]), int(fin[2]), int(fin[3]), int(fin[4]), int(fin[5]))

difference = (fin - debut).seconds

write["cur_bytes"] = write["cur_bytes"].astype(int) 
write["bytes"] = write["bytes"].astype(int) 
write["durée"] = write["durée"].astype(float) 

new_write = write.groupby("cluster").sum()

new_write["nb_occurence"] = ( write.groupby("cluster").count() )["durée"]
new_write = new_write.drop("offset", axis=1)

new_write = new_write.reset_index()

new_write["read_intensity(ops)"] = new_write["nb_occurence"]/difference
new_write["débit(o/s)"] = new_write["cur_bytes"]/new_write["durée"]

#new_write = new_write.T
if os.path.exists("./simulation/files/json/write_log.json"):
	os.remove("./simulation/files/json/write_log.json")
new_write.to_json("./simulation/files/json/write_log.json", orient='records', force_ascii=False)
