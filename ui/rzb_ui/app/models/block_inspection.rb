class BlockInspection < ActiveRecord::Base
	set_table_name 'Block_Inspection'
	set_primary_key 'Status'

	belongs_to :block, :foreign_key => 'Block_ID'
	belongs_to :nugget, :foreign_key => 'Inspector_UUID'
	belongs_to :app_type, :foreign_key => 'Inspector_Type_UUID'

	def self.DONE
		0
	end

	def self.ERROR
		2
	end

	def self.DEFERRED
		3
	end

	def self.PENDING
		4
	end

	def done?
		self.Status == BlockInspection.DONE
	end

	def error?
		self.Status == BlockInspection.ERROR
	end

	def deferred?
		self.Status == BlockInspection.DEFERRED
	end

	def pending?
		self.Status == BlockInspection.PENDING
	end

	def status_string
		return "done" if self.done?
		return "error" if self.error?
		return "deferred" if self.deferred?
		return "pending" if self.pending?
	end
end
