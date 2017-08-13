class Metabook < ActiveRecord::Base
	set_table_name 'Metabook'
	set_primary_key 'Metabook_ID'
	
	has_many :blocks
	has_many :metadatas, :dependent => :destroy
	has_many :events
	has_many :alerts
	
end
