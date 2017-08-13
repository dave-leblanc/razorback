class Block < ActiveRecord::Base
	set_table_name 'Block' 
	set_primary_key 'Block_ID'

	belongs_to :data_type, :foreign_key => 'Data_Type_UUID'
	belongs_to :metabook, :foreign_key => 'Metabook_ID'
	has_and_belongs_to_many :blocks, :class_name => "Block", :join_table => 'LK_Block_Block', :foreign_key => 'Parent_ID', :association_foreign_key => 'Child_ID',
		:finder_sql => 'SELECT child.* FROM `Block` as child left join `LK_Block_Block` on (Block_ID = Child_ID) WHERE Parent_ID = #{id}'

	has_many :metadatas, :through => :metabook, :foreign_key => 'Metabook_ID'
	has_many :events
	has_many :alerts
	has_many :block_inspections

	def destroy	
		connection.delete("DELETE FROM LK_Block_Block WHERE Child_ID = #{self.id}")
		self.blocks.each {|b| b.destroy}
		self.metabook.metadatas.each {|m| m.destroy}
		self.events.each {|a| a.destroy}
		self.block_inspections.each {|b| b.destroy}
		super
		self.metabook.destroy
	end

	def self.good
		0x00000001
	end

	def self.bad
		0x00000002
	end

	def self.pending
		0x00000040
	end

	def self.dodgy
		0x00000080
	end

	def suspicious?
		self.dodgy? && !self.bad?
	end

	def dodgy?
		(self.SF_Flags.to_i & Block.dodgy) != 0
	end

	def pending?
		(self.SF_Flags.to_i & Block.pending) != 0
	end

	def good?
		(self.SF_Flags.to_i & Block.good) != 0
	end

	def bad?
		(self.SF_Flags.to_i & Block.bad) != 0
	end

	def unknown?
		!self.good? && !self.bad?
	end

	def result
		return Block.dodgy if self.suspicious?
		return Block.bad if self.bad?
		return Block.good if self.good?
	end

	def result_string
		return "Suspicious" if self.suspicious?
		return "Bad" if self.bad?
		return "Good" if self.good?
		return "Pending" if self.pending?
		return "Unknown" if self.unknown?
	end

	def hash_digest
		self.Hash.unpack("H*").to_s
	end

	def self.convert_hash(hash)
		hash.to_a.pack("H*")
	end
	
	def display_id_short
		"#{self.hash_digest[0..6]}-#{self.Size}"
	end

	def display_id
		"#{self.hash_digest}-#{self.Size}"
	end

    def hash_digest_short
		return "#{self.hash_digest[0..6]}"
    end

	def sf_flags_string
		"0x%08x" % self.SF_Flags.to_i
	end

	def ent_flags_string
		"0x%08x" % self.ENT_Flags.to_i
	end

	def self.find_by_hash_digest(hash)
		Block.find_by_Hash(Block.convert_hash(hash))
	end

	def self.find_by_hash_digest!(hash)
		Block.find_by_Hash!(Block.convert_hash(hash))
	end

	def filenames
		self.filename_metadatas.map { |m| m.Metadata }.uniq
	end

	def all_blocks
		blocks = self.blocks.dup
		self.blocks.map {|b| b.all_blocks }.each {|b| blocks += b } 

		return blocks.uniq
	end

	def all_alerts 
		alerts = self.alerts.dup
		self.alerts.map {|a| a.all_alerts }.each {|a| alerts += a}

		return alerts.uniq
	end

	def file_path
		"#{ConfigOption.RAZORBACK_BLOCK_FILE_PATH.value}/#{self.hash_digest[0].chr}/#{self.hash_digest[1].chr}/#{self.hash_digest[2].chr}/#{self.hash_digest[3].chr}/#{self.hash_digest}.#{self.Size}"
	end

	def file_extension
		return File.extname(self.filenames.first) unless self.filenames.empty?
	end

	def filename_metadatas
		metadatas = []
        self.events.sort{|y,x| x.Time_Secs <=> y.Time_Secs }.each do |e| 
			metadatas += e.metadatas.find_all_by_Metadata_Name_UUID(MetadataName.FILENAME.UUID)
		end

		return metadatas
	end

	def malwarename_metadatas
		metadatas = []
        self.alerts.sort{|y,x| x.Time_Secs <=> y.Time_Secs }.each do |e| 
			metadatas += e.metadatas.find_all_by_Metadata_Name_UUID(MetadataName.MALWARENAME.UUID)
		end

		return metadatas
	end

end
