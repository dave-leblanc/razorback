class DetectionScriptsController < ApplicationController
    active_scaffold :detection_script do |config|
		config.columns = [:name, :upload_path, :post_command]
        config.label = "Script Configuration"

        config.actions.exclude :search
        config.actions.exclude :show
    end
end
