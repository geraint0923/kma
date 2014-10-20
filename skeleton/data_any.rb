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
puts "Max Rate #{max_idx} => #{max_rate}"
puts "Min Rate #{min_idx} => #{min_rate}"
puts "Average Rate #{total/count}"

malloc_best = 99999999999999999
free_best = 99999999999999999
malloc_worst = 0
free_worst = 0
malloc_time = 0
free_time = 0
malloc_count = 0
free_count = 0

File.open("kma_timing.dat").each do |line|
	sl = line.split
	cmd = sl[0]
	timing = sl[1].to_i
	if cmd == "REQUEST"
		malloc_best = timing if timing < malloc_best
		malloc_worst = timing if timing > malloc_worst
		malloc_time += timing
		malloc_count += 1
	elsif cmd == "FREE"
		free_best = timing if timing < free_best
		free_worst = timing if timing > free_worst
		free_time += timing
		free_count += 1
	end
	
end

puts "Malloc best #{malloc_best} worst #{malloc_worst} average #{malloc_time/malloc_count}"
puts "Free best #{free_best} worst #{free_worst} average #{free_time/free_count}"
