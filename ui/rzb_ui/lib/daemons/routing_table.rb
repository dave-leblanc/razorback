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
				RoutingInstance.delete_all

				conn.cmd(ConfigOption.RAZORBACK_ROUTING_ROUTING_TABLE_COMMAND.value) do |response|
					response.split(/\n/).each do |line|

						if line =~ /^([^\s]+)\s+([^\s]+)\s+$/
							app_type = AppType.find_by_Name($2)
							data_type = DataType.find_by_Name($1)

							RoutingInstance.create(:app_type => app_type, :data_type => data_type)
						end
					end
				end
			
				sleep 10
			end

		rescue Errno::EPIPE
			# Ignore and let's restart the connection
		end

		conn.close()

	rescue Errno::ECONNREFUSED
		sleep 10
	end
end
