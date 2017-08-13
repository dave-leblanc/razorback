class EventsController < ApplicationController
	active_scaffold :events do |config|
		config.columns = [:Time_Secs, :size, :data_type, :hash_digest, :block_alerts, :blocks, :metadata_count, :result]
		config.list.label = 'Events'
		config.list.per_page = 100
		config.list.sorting = {:Time_Secs => 'DESC'}
	
		# Search
		config.search.columns = [:data_type, :size, :hash_digest, :block_alerts]

		# Actions
		config.actions.exclude :create
		config.actions.exclude :delete
		config.actions.exclude :show
		config.actions.exclude :update

		# block_alerts
		config.columns[:block_alerts].includes = [:alerts]
		config.columns[:block_alerts].label = 'Alerts'
		config.columns[:block_alerts].search_sql = 'Alert.Message'
		config.columns[:block_alerts].set_link 'block_alerts'
		config.columns[:block_alerts].sort_by :sql => 
			'(SELECT COUNT(Alert.Alert_ID) FROM Alert WHERE Event.Block_ID = Alert.Block_ID)'

		# data_type
		config.columns[:data_type].search_sql = 'UUID_Data_Type.Name'
		config.columns[:data_type].sort_by :sql => 'UUID_Data_Type.Name'

		# hash_digest
		config.columns[:hash_digest].search_sql = 'HEX(Block.Hash)'
		config.columns[:hash_digest].sort_by :sql => 'Block.Hash'

		# metadata_count
		config.columns[:metadata_count].label = 'Metadata'
		config.columns[:metadata_count].set_link 'metadata'
		config.columns[:metadata_count].sort_by :sql => 
			'(SELECT COUNT(Metadata.Metadata_ID) FROM Metabook, Metadata WHERE Event.Metabook_ID = Metabook.Metabook_ID AND Metabook.Metabook_ID = Metadata.Metabook_ID)'

		# result
		config.columns[:result].includes = [{:block => [:data_type]}]
		config.columns[:result].sort_by :sql => 'Block.SF_Flags'
		
		# size
		config.columns[:size].search_sql = 'Block.Size'
		config.columns[:size].sort_by :sql => 'Block.Size'

		# Time_Secs
		config.columns[:Time_Secs].label = 'Created'
		
	end

	def conditions_for_collection
		if params[:all].nil?
			['Block.SF_Flags & ?', Block.bad]		
		end
	end

	def block_alerts
		@event = Event.find(params[:id])
	
		render :layout => false
	end

	def metadata
		@event = Event.find(params[:id])

		render :layout => false
	end
end
