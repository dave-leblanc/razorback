class AlertsController < ApplicationController
	active_scaffold :alerts do |config|
		config.columns = [:Priority, :nugget, :Message, :metadata_count]
		config.show.columns = [:Priority, :Message, :GID, :SID, :Time_Secs, :block]
			
		config.columns[:nugget].label = "Inspector"

        config.actions.exclude :delete
        config.actions.exclude :create
        config.actions.exclude :update
        config.actions.exclude :search
        config.actions.exclude :show
		config.label = ""

		config.columns[:metadata_count].set_link "metadata"
		config.columns[:metadata_count].label = "Metadata"
		config.columns[:Time_Secs].label = "Created"
		config.list.pagination = false
	end

	def metadata
		@alert = Alert.find(params[:id])
		
		render :layout => false
	end
end
