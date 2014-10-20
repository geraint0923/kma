#!/usr/bin/env ruby


`./kma_#{ARGV[0]} ./testsuite/5.trace`

max_rate = 0
max_idx = -1
min_rate = 1
min_idx = -1
total = 0
count = 0
idx = 0

File.open("kma_output.dat").each do |line| 
	sl = line.split
	used = sl[1].to_f
	allocated = sl[2].to_f
	rate = used / allocated;
	if rate > max_rate
		max_rate = rate
		max_idx = idx
	end
	if rate < min_rate
		min_rate = rate
		min_idx = idx
	end
	if allocated != 0
		total += rate
		count += 1
	end
	idx += 1
#	puts "used #{used}    allocated #{allocated} max_rate #{max_rate} min_rate #{min_rate}"
end
puts "total #{total} idx #{idx} average #{total/idx}"
puts "max_rate #{max_idx} => #{max_rate} ||  min_rate #{min_idx} => #{min_rate}"
