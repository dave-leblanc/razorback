class Event < ActiveRecord::Base
	set_table_name 'Event'
	set_primary_key 'Event_ID'

	belongs_to :metabook, :foreign_key => 'Metabook_ID'
	belongs_to :block, :foreign_key => 'Block_ID'
	belongs_to :nugget, :foreign_key => 'Nugget_UUID'

	has_many :metadatas, :through => :metabook, :foreign_key => 'Metabook_ID'
	has_many :alerts, :foreign_key => 'Event_ID'

	def metadata_search
		self.metadatas
	end
	
	def block_alerts
		self.block ? self.block.alerts : []
	end

	def destroy
		self.metabook.metadatas.each {|m| m.destroy}
		self.alerts.each {|e| e.destroy}
		super
		self.metabook.destroy
	end

	def created_at
		Time.at(self.Time_Secs)
	end

	def result
		self.block ? self.block.result_string : ""
	end

	def size
		self.block ? self.block.Size : 0
	end

	def filename
		begin
			return self.metadatas.find(:all, :conditions => ['Metadata_Name_UUID IN (?)', [MetadataName.FILENAME.UUID, MetadataName.URI.UUID]]).first.Metadata
		rescue Exception => e
			return self.block.hash_digest
		end
	end
end
