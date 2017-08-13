class MetadatasController < ApplicationController
	active_scaffold :metadatas do |config|
		config.columns = [:name, :metadata]
		config.label = ''

		config.actions.exclude :delete
		config.actions.exclude :update
		config.actions.exclude :create
		config.actions.exclude :show
		config.actions.exclude :search
		config.list.pagination = false
	end

	def report
		@metadata = Metadata.find(params[:id])

		render :layout => false
	end
end
