class NuggetStatus < ActiveRecord::Base
	belongs_to :nugget, :foreign_key => 'Nugget_UUID'
	validates_presence_of :status
end
