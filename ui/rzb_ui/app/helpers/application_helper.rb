# Methods added to this helper will be available to all templates in the application.
module ApplicationHelper

	def active_scaffold_column_text(column, record)
		truncate(clean_column_value(record.send(column.name)), :length => 100)
	end
end
