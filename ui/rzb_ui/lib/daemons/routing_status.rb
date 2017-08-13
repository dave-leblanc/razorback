#!/usr/bin/env ruby

require File.join(File.dirname(__FILE__), "..", "..", "config", "environment.rb")
require 'file-tail'

$running = true
Signal.trap("TERM") do
  $running = false
end

while($running) do
	begin
		File.open(ConfigOption.RAZORBACK_ROUTING_STATS_PATH.value) do |log|
			log.extend(File::Tail)
			log.interval = ConfigOption.RAZORBACK_CONSOLE_STATUS_UPDATE_INTERVAL.value.to_i
			log.backward(0)

			log.tail do |line|
				break unless $running
				
				if line =~ /^R-DTC/				
					line.split(/, /).each do |stat|
						(dt, count) = stat.split(/: /)
						next unless dt && count	

						dt.gsub!(/^R-/, '')
						data_type = DataType.find_by_Name(dt)
						next if data_type.nil?	

						data_type.routing_count = RoutingCount.new if data_type.routing_count.nil?
						
						if data_type.routing_count.count != count
							data_type.routing_count.count = count
							data_type.routing_count.save
						end

					end
				end
			end
		end

	rescue Errno::ENOENT
		sleep ConfigOption.RAZORBACK_CONSOLE_STATUS_UPDATE_INTERVAL.value.to_i
	end
end
