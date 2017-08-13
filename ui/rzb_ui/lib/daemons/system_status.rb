#!/usr/bin/env ruby

require File.join(File.dirname(__FILE__), "..", "..", "config", "environment.rb")

$running = true
Signal.trap("TERM") do 
  $running = false
end

while($running) do

   SystemStatus.all.each do |system|
      if system.command && system.confirmation
			status = "error"

			io = IO.popen(system.command)

         io.each_line do |line|
            if line =~ /#{system.confirmation}/
               status = "good"
               break
            end
         end

			io.close

			if status != system.status
				system.status = status
         	system.save
			end
      end
   end

	sleep ConfigOption.RAZORBACK_CONSOLE_STATUS_UPDATE_INTERVAL.value.to_i	
end
