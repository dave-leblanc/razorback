class ConfigurationController < ApplicationController
	active_scaffold :config_option do |config|
		config.actions.exclude :show
		config.actions.exclude :search
		config.actions.exclude :create
		config.actions.exclude :delete
		config.label = 'Global Configuration'
		config.list.per_page = 1000

		config.columns = [:name, :value]

		config.columns[:name].label = 'Option'
	end
end
