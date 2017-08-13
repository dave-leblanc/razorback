#!/usr/bin/env ruby

require File.join(File.dirname(__FILE__), "..", "..", "config", "environment.rb")
require 'net/telnet'

$running = true
Signal.trap("TERM") do 
  $running = false
end

while($running) do
	begin
		conn = Net::Telnet::new(
			"Host" => ConfigOption.RAZORBACK_CONSOLE_HOSTNAME.value,
			"Port" => ConfigOption.RAZORBACK_CONSOLE_PORT.value,
			"Timeout" => 10
		)

		conn.puts(ConfigOption.RAZORBACK_CONSOLE_USERNAME.value)
		conn.puts(ConfigOption.RAZORBACK_CONSOLE_PASSWORD.value)

		begin
			while($running) do
				nuggets = []

				conn.cmd(ConfigOption.RAZORBACK_CONSOLE_STATUS_COMMAND.value) do |response|
					response.split(/\n/).each do |line|

						if line =~ /([^\s]+)\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)/
							uuid = $3
							status = $4
							deadtime = $5

							if $1 !~ /Nugget/
								nugget = Nugget.find_by_uuid_string(uuid)

								if nugget.nugget_status.nil?
									nugget.nugget_status = NuggetStatus.new
								end

								# Ignore deadtime for now
								# nugget.nugget_status.deadtime = deadtime
								
								if nugget.nugget_status.status != status
									nugget.nugget_status.status = status
									nugget.nugget_status.save

								end

								nuggets << nugget
							end
						end
					end
				end

				if nuggets.empty?
					old_nuggets = Nugget.all
				else
					old_nuggets = Nugget.find(:all, :conditions => ['UUID NOT IN (?)', nuggets.map {|n| n.UUID}])
				end
			
				old_nuggets.each do |nugget|
					next if nugget.nugget_status.nil?

					nugget.nugget_status.status = "Terminated"
					nugget.nugget_status.save
				end


				sleep ConfigOption.RAZORBACK_CONSOLE_STATUS_UPDATE_INTERVAL.value.to_i
			end

		rescue Errno::EPIPE
			# Ignore and let's restart the connection
		end

		conn.close()

	rescue Errno::ECONNREFUSED

		# Set all nuggets to unknown status
		Nugget.all.each do |nugget|
			unless nugget.nugget_status.nil?
				nugget.nugget_status.status = "alert"
				nugget.nugget_status.save
			end
		end
		
		sleep ConfigOption.RAZORBACK_CONSOLE_STATUS_UPDATE_INTERVAL.value.to_i
	end
end
