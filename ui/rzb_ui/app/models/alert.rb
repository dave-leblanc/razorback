class Alert < ActiveRecord::Base
	set_table_name 'Alert'
	set_primary_key 'Alert_ID'

	belongs_to :block, :foreign_key => 'Block_ID'
	belongs_to :event, :foreign_key => 'Event_ID'
	belongs_to :nugget, :foreign_key => 'Nugget_UUID'
	belongs_to :metabook, :foreign_key => 'Metabook_ID'

	has_many :metadatas, :through => :metabook, :foreign_key => 'Metabook_ID'

	def destroy
		self.metabook.metadatas.each {|m| m.destroy}
		super
		self.metabook.destroy
	end

	def report
		self.metadatas.each do |metadata|
			return metadata.Metadata if metadata.metadata_name == MetadataName.REPORT
		end
	end
end
