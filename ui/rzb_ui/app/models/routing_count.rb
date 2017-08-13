class RoutingCount < ActiveRecord::Base
	belongs_to :data_type, :foreign_key => 'Data_Type_UUID'
end
