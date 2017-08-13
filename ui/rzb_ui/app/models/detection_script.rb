class DetectionScript < ActiveRecord::Base
	def self.YARA
		DetectionScript.find_by_name('Yara')
	end

	def self.CLAMAV
		DetectionScript.find_by_name('ClamAV')
	end
end
