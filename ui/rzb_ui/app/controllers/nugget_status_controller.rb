class NuggetStatusController < ApplicationController

	active_scaffold :nugget_status do |config|
		config.label = ''
		config.columns = [:nugget, :status]

		config.actions.exclude :show
		config.actions.exclude :delete
		config.actions.exclude :update
		config.actions.exclude :create
		config.actions.exclude :search
	
		config.list.pagination = false

		#config.columns[:nugget].includes = [:nugget]
		#config.columns[:nugget].sort_by :sql => 'Nugget.Name'
		#config.list.sorting = { :status => :desc }
		config.columns[:status].sort_by nil
		config.columns[:nugget].sort_by nil


	end

	def sorting
		if params[:sort]
			session[:nugget_status_sort] = params[:sort]
		end

		if params[:sort_direction]
			session[:nugget_status_sort_direction] = params[:sort_direction]
		end
	end

	def refresh
		render :layout => false
	end

	def check_updates
		begin
			if NuggetStatus.count(:all, :conditions => ['updated_at > ?', Time.at(params[:last_updated].to_f).utc]) > 0
				render :text => 'true'
			else
				render :text => 'false'
			end

		rescue Exception => e
			logger.info "Exception in nugget_status.check_updates: #{e.to_s}"
			render :text => false
		end
	end
end
