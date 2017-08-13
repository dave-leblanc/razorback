class MetadataName < ActiveRecord::Base
	set_table_name 'UUID_Metadata_Name'
	set_primary_key 'UUID'

	has_many :metadatas

	def self.FILENAME
		MetadataName.find_by_Name('FILENAME')
	end

	def self.URI
		MetadataName.find_by_Name('URI')
	end

	def self.MALWARENAME
		MetadataName.find_by_Name('MALWARENAME')
	end

	def self.REPORT
		MetadataName.find_by_Name('REPORT')
	end

	def hex_digest
		self.UUID.unpack("H*").to_s
	end

end
