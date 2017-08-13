class SystemStatusController < ApplicationController

	active_scaffold :system_status do |config|
		config.columns = [:name, :status]

		config.label = ''
		config.actions.exclude :show
		config.actions.exclude :create
		config.actions.exclude :update
		config.actions.exclude :delete
		config.actions.exclude :search
		config.list.pagination = false

		#config.list.sorting = { :name => :asc }
		config.columns[:name].sort_by  nil
		config.columns[:status].sort_by nil
	end

    def sorting
        if params[:sort]
            session[:system_status_sort] = params[:sort]
        end

        if params[:sort_direction]
            session[:system_status_sort_direction] = params[:sort_direction]
        end
    end

    def refresh
        render :layout => false
    end

   def check_updates
      begin
         if SystemStatus.count(:all, :conditions => ['updated_at > ?', Time.at(params[:last_updated].to_f).utc]) > 0
            render :text => 'true'
         else
            render :text => 'false'
         end

      rescue Exception => e
         logger.info "Exception in system_status.check_updates: #{e.to_s}"
         render :text => false
      end
   end

end
