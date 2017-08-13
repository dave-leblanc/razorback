class AppType < ActiveRecord::Base
	set_table_name 'App_Type'
	set_primary_key 'UUID'

	has_many :nuggets
	has_many :block_inspections
	has_many :routing_instances, :foreign_key => 'App_Type_UUID'

	def to_s
		self.Description
	end
end
