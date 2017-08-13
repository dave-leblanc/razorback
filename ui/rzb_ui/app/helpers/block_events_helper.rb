module BlockEventsHelper
    def Time_Secs_column(record)
        Time.at(record.Time_Secs).strftime("%m/%d/%Y %H:%M:%S")
    end

	def nugget_column(record)
		record.nugget.Name
	end

    def metadata_count_column(record)
        record.metadatas.size
    end

end
