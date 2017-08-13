class BlockInspectionsController < ApplicationController
	active_scaffold :block_inspections do |config|
		config.columns = [:app_type, :nugget_name, :status]

		config.columns[:status].sort_by :sql => 'Status'
		config.columns[:nugget_name].sort_by :method => 'nugget.Name'

		config.actions.exclude :update
		config.actions.exclude :show
		config.actions.exclude :delete
		config.actions.exclude :create
		config.actions.exclude :search

		config.list.pagination = false
		config.label = ""

	end
end
