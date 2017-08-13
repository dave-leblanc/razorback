
module EventsHelper

	def Time_Secs_column(record)
		Time.at(record.Time_Secs).strftime("%m/%d/%Y %H:%M:%S")
	end

	def blocks_column(record)
		record.block.all_blocks ? record.block.all_blocks.size : 0
	end

	def data_type_column(record)
		record.block.data_type.Name
	end

	def hash_digest_column(record)
		filename = record.filename
		filename = "#{filename[0..75]} ..." if filename.size > 75

		(link_to filename, { :controller => 'blocks', :action => 'report', :id => record.block.id })  + "<br />&nbsp;&nbsp;&nbsp;" + 
		(link_to record.block.hash_digest, { :controller => 'blocks', :action => 'report', :id => record.block.id })
	end

	def list_row_class(record)
		record.block.result_string.downcase
	end

	def result_column(record)
	end

	def metadata_count_column(record)
		record.metadatas ? record.metadatas.size : 0
	end

	def block_alerts_column(record)
		record.block_alerts.size
	end
	
end
