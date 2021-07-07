import sys

def average(read_latency, how_many):
	sum =0
	for i in range(0,how_many):
		sum+=read_latency[0]
	return (sum/how_many)



if __name__ == '__main__':

# storing the arguments
	func = ('['+ str(sys.argv[1])+']')
	loc_func = str(sys.argv[2])

	read_latency = [0 for i in range(0,40000)]
	how_many = 0

	file = open(loc_func)
	for line in file:
		col = 0
		line.strip().split('/n')
		if func in line:
			how_many+=1
	    	#print (line)
			end_time = 0
			start_time = 0
			for word in line.split():
	    		#print(word)
				col+=1
				if col == 7:
					start_time = int(word)
					#print(start_time)
				elif col ==8:
					end_time = int(word)
					#print(end_time)
					time_spent = int(end_time-start_time)
					#print(time_spent)
					read_latency[how_many-1]=time_spent
	file.close()
	avg_latency = average(read_latency, how_many)
	print ("Average latency : ", avg_latency," ns and ", avg_latency/1000000, " ms")
	print ("function occurrence : ", how_many)