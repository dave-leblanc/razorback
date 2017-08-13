
class Nugget < ActiveRecord::Base
	set_table_name 'Nugget'
	set_primary_key 'UUID'

	has_many :events
	has_many :alerts
	has_many :block_inspections, :foreign_key => 'Inspector_UUID'
    has_one :nugget_status, :foreign_key => 'Nugget_UUID'
	
	belongs_to :app_type, :foreign_key => 'App_Type_UUID'
	belongs_to :nugget_type, :foreign_key => 'Nugget_Type_UUID'


	def to_s
		self.Name
	end

	def self.find_by_uuid_string(uuid)
		Nugget.find_by_UUID(uuid)
	end
end
