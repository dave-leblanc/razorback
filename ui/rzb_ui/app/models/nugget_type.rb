class NuggetType < ActiveRecord::Base
	set_table_name 'Nugget_Type'
	set_primary_key 'UUID'

	has_many :nuggets
end
