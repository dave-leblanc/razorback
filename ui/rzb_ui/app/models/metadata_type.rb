class MetadataType < ActiveRecord::Base
	set_table_name 'UUID_Metadata_Type'
	set_primary_key 'UUID'

	has_many :metadatas

	def self.STRING
		MetadataType.find_by_Name('STRING')
	end
	
	def self.PORT
		MetadataType.find_by_Name('PORT')
	end
	
	def self.IPv4_ADDR
		MetadataType.find_by_Name('IPv4_ADDR')
	end
	
	def self.IPv6_ADDR
		MetadataType.find_by_Name('IPv6_ADDR')
	end
	
	def self.IP_PROTO
		MetadataType.find_by_Name('IP_PROTO')
	end
end
