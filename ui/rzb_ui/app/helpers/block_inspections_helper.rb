module BlockInspectionsHelper
	def app_type_column(record)
		record.app_type
	end

	def status_column(record)
		record.status_string.capitalize
	end

	def nugget_name_column(record)
		record.nugget
	end
end
