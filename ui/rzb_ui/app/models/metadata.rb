require 'ipaddr'

class Metadata < ActiveRecord::Base
	set_table_name 'Metadata'
	set_primary_key 'Metadata_ID'

	belongs_to :metabook, :foreign_key => 'Metabook_ID'
	belongs_to :metadata_type, :foreign_key => 'Metadata_Type_UUID'
	belongs_to :metadata_name, :foreign_key => 'Metadata_Name_UUID'

	def  to_s

		begin
			if self.metadata_type == MetadataType.STRING
				self.Metadata
			elsif self.metadata_type == MetadataType.IPv4_ADDR
				IPAddr.new(self.Metadata.unpack('C*').map{|b| "%02x" % b}.join('').to_i(16), Socket::AF_INET).to_s
			elsif self.metadata_type == MetadataType.IPv6_ADDR
				IPAddr.new(self.Metadata.unpack('C*').map{|b| "%02x" % b}.join('').to_i(16), Socket::AF_INET6).to_s
			elsif self.metadata_type == MetadataType.PORT
				((self.Metadata[0].ord * 256) + self.Metadata[1].ord).to_s
			else
				""
			end

		rescue Exception => e
			return ""
		end
	end
end
