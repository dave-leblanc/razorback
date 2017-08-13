class ConfigOption < ActiveRecord::Base
    before_destroy :destroy_required
	before_update :update_required

	def destroy_required
		if self.required
			errors.add "Required", "options can not be destroyed"
		end
	end

	def update_required
		if self.required && (ConfigOption.find_by_name(self.name).nil? || (self.name != ConfigOption.find_by_name(self.name).name))
			errors.add "Required", "option names can not be changed"
			return false
		end
	end

	def self.RAZORBACK_ROUTING_ROUTING_TABLE_COMMAND
		ConfigOption.find_by_name('RAZORBACK_ROUTING_ROUTING_TABLE_COMMAND')
	end

	def self.RAZORBACK_PATH
		ConfigOption.find_by_name('RAZORBACK_PATH')
	end

	def self.RAZORBACK_BLOCK_FILE_PATH
		ConfigOption.find_by_name('RAZORBACK_BLOCK_FILE_PATH')
	end

	def self.RAZORBACK_CONSOLE_USERNAME
		ConfigOption.find_by_name('RAZORBACK_CONSOLE_USERNAME')
	end

	def self.RAZORBACK_CONSOLE_PASSWORD
		ConfigOption.find_by_name('RAZORBACK_CONSOLE_PASSWORD')
	end

	def self.RAZORBACK_CONSOLE_HOSTNAME
		ConfigOption.find_by_name('RAZORBACK_CONSOLE_HOSTNAME')
	end

	def self.RAZORBACK_CONSOLE_PORT
		ConfigOption.find_by_name('RAZORBACK_CONSOLE_PORT')
	end

	def self.RAZORBACK_CONSOLE_STATUS_COMMAND
		ConfigOption.find_by_name('RAZORBACK_CONSOLE_STATUS_COMMAND')
	end

	def self.RAZORBACK_CONSOLE_STATUS_UPDATE_INTERVAL
		ConfigOption.find_by_name('RAZORBACK_CONSOLE_STATUS_UPDATE_INTERVAL')
	end

	def self.RAZORBACK_ROUTING_STATS_PATH
		ConfigOption.find_by_name('RAZORBACK_ROUTING_STATS_PATH')
	end
end
