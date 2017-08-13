module NuggetStatusHelper
	def updated_at_column(record)
		record.updated_at.strftime("%m/%d/%Y %H:%M:%S")
	end

    def status_column(record)
    end

    def list_row_class(record)
        record.status.downcase if record.status
    end

end
