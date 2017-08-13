class SystemStatusConfigurationController < ApplicationController
   active_scaffold :system_status do |config|
      config.columns = [:name, :command, :confirmation]
      config.label = "System Status Configuration"

      config.actions.exclude :search
      config.actions.exclude :show
      
   end
end
