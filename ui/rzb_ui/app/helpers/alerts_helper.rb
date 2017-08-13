module AlertsHelper
    def Time_Secs_column(record)
        Time.at(record.Time_Secs).strftime("%m/%d/%Y %H:%M:%S")
    end

	def nugget_column(record)
		record.nugget.app_type.Description
	end

	def metadata_count_column(record)
		record.metadatas.size
	end

end
