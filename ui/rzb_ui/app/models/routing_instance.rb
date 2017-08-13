class RoutingInstance < ActiveRecord::Base
	belongs_to :data_type, :foreign_key => 'Data_Type_UUID'
	belongs_to :app_type, :foreign_key => 'App_Type_UUID'
end
