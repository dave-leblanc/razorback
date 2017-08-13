class RoutingCountsController < ApplicationController
	active_scaffold :routing_counts do |config|
		config.columns = [:data_type, :count]
		config.actions.exclude :create
		config.actions.exclude :search
		config.actions.exclude :update
		config.actions.exclude :delete
		config.actions.exclude :show
		config.columns[:data_type].sort_by nil
		config.columns[:count].sort_by nil
	
		config.list.pagination = false
		config.label = ''
	end

	def refresh
      render :layout => false
   end
	
   def check_updates
      begin
         if RoutingCount.count(:all, :conditions => ['updated_at > ?', Time.at(params[:last_updated].to_f).utc]) > 0
            render :text => 'true'
         else
            render :text => 'false'
         end

      rescue Exception => e
         logger.info "Exception in routing_counts.check_updates: #{e.to_s}"
         render :text => false
      end
   end

end
