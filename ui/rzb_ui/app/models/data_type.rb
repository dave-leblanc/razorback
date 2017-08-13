class DataType < ActiveRecord::Base
	set_table_name 'UUID_Data_Type'
	set_primary_key 'UUID'

	has_many :routing_instances, :foreign_key => 'Data_Type_UUID'
	has_many :blocks, :foreign_key => 'Data_Type_UUID'
	has_one :routing_count, :foreign_key => 'Data_Type_UUID'

	def to_s
		self.Name
	end
end
