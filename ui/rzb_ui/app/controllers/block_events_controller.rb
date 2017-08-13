class BlockEventsController < ApplicationController
    active_scaffold :events do |config|
        config.columns = [:Time_Secs, :nugget, :metadata_count]
        config.list.label = ''
        config.list.per_page = 15

        config.columns[:Time_Secs].label = 'Created'
        config.columns[:metadata_count].set_link 'metadata'
        config.columns[:metadata_count].label = 'Metadata'

        config.actions.exclude :show
        config.actions.exclude :delete
        config.actions.exclude :create
        config.actions.exclude :update
        config.actions.exclude :search

        list.sorting = {:Time_Secs => 'DESC'}

    end

    def metadata
        @event = Event.find(params[:id])

        render :layout => false
    end

end
